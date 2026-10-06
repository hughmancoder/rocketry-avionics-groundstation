#include "Can.h"
#include "main.h"
#include "telemetry_packet.h"
#include <stdint.h>
#include <string.h>

extern CAN_HandleTypeDef hcan1;

#define ALL_CHUNKS_MASK ((1U << TELEMETRY_NUM_CHUNKS) - 1U)

// Reassembly state (touched only from the CAN RX interrupt)
static uint8_t assembly[TELEMETRY_PACKET_SIZE];
static uint32_t chunk_mask = 0;

// Last complete packet, handed to the main loop
static TelemetryPacket latest_pkt;
static volatile bool latest_is_new = false;

volatile uint32_t can_frame_count = 0;
volatile uint32_t can_packet_count = 0;
volatile uint32_t can_drop_count = 0;

HAL_StatusTypeDef Telemetry_CAN_Init(void) {
  CAN_FilterTypeDef filter = {0};
  HAL_StatusTypeDef status;

#if CAN_LOOPBACK_TEST
  // Re-initialise in silent loopback; FIFO TX priority keeps chunks in order
  hcan1.Init.Mode = CAN_MODE_SILENT_LOOPBACK;
  hcan1.Init.TransmitFifoPriority = ENABLE;
  status = HAL_CAN_Init(&hcan1);
  if (status != HAL_OK) {
    return status;
  }
#endif

  // Accept every standard-ID frame into FIFO0; chunk IDs are checked in the
  // callback (the 0xAA.. range isn't mask-aligned)
  filter.FilterBank = 0;
  filter.FilterMode = CAN_FILTERMODE_IDMASK;
  filter.FilterScale = CAN_FILTERSCALE_32BIT;
  filter.FilterIdHigh = 0x0000;
  filter.FilterIdLow = 0x0000;
  filter.FilterMaskIdHigh = 0x0000;
  filter.FilterMaskIdLow = 0x0000;
  filter.FilterFIFOAssignment = CAN_RX_FIFO0;
  filter.FilterActivation = CAN_FILTER_ENABLE;

  status = HAL_CAN_ConfigFilter(&hcan1, &filter);
  if (status != HAL_OK) {
    return status;
  }

  HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);

  status = HAL_CAN_Start(&hcan1);
  if (status != HAL_OK) {
    return status;
  }
  return HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

bool Telemetry_CAN_GetLatest(TelemetryPacket *out) {
  __disable_irq();
  memcpy(out, &latest_pkt, sizeof(*out));
  bool is_new = latest_is_new;
  latest_is_new = false;
  __enable_irq();
  return is_new;
}

#if CAN_LOOPBACK_TEST
HAL_StatusTypeDef Telemetry_CAN_SendMockPacket(void) {
  static uint32_t mock_num = 0;
  TelemetryPacket pkt = {0};

  // Plausible flight computer values that change between packets
  float t = HAL_GetTick() / 1000.0f;
  pkt.header = TELEMETRY_PACKET_HEADER;
  pkt.packet_num = ++mock_num;
  pkt.timestamp_ms = HAL_GetTick();
  pkt.imu_accel[2] = 1.0f;
  pkt.temperature_C = 24.0f + (float)(mock_num % 10) * 0.1f;
  pkt.pressure_hPa = 1013.25f - t * 0.01f;
  pkt.altitude_m = 50.0f + t * 0.1f;

  CAN_TxHeaderTypeDef header = {0};
  header.IDE = CAN_ID_STD;
  header.RTR = CAN_RTR_DATA;

  const uint8_t *bytes = (const uint8_t *)&pkt;
  for (uint32_t i = 0; i < TELEMETRY_NUM_CHUNKS; i++) {
    uint32_t offset = i * TELEMETRY_CHUNK_BYTES;
    uint32_t len = TELEMETRY_PACKET_SIZE - offset;
    if (len > TELEMETRY_CHUNK_BYTES) {
      len = TELEMETRY_CHUNK_BYTES;
    }
    header.StdId = TELEMETRY_PACKET_HEADER + i;
    header.DLC = len;

    uint32_t start = HAL_GetTick();
    while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) == 0) {
      if (HAL_GetTick() - start > 10) {
        return HAL_TIMEOUT;
      }
    }
    uint32_t mailbox;
    HAL_StatusTypeDef status = HAL_CAN_AddTxMessage(
        &hcan1, &header, (uint8_t *)&bytes[offset], &mailbox);
    if (status != HAL_OK) {
      return status;
    }
  }
  return HAL_OK;
}
#endif

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  uint8_t payload[8];
  CAN_RxHeaderTypeDef header;
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, payload) != HAL_OK) {
    return;
  }

  if (header.IDE != CAN_ID_STD || header.RTR != CAN_RTR_DATA ||
      header.StdId < TELEMETRY_PACKET_HEADER ||
      header.StdId >= TELEMETRY_PACKET_HEADER + TELEMETRY_NUM_CHUNKS) {
    return; // not a telemetry chunk
  }
  can_frame_count++;

  uint32_t chunk_index = header.StdId - TELEMETRY_PACKET_HEADER;
  uint32_t offset = chunk_index * TELEMETRY_CHUNK_BYTES;
  uint32_t expected_len = TELEMETRY_PACKET_SIZE - offset;
  if (expected_len > TELEMETRY_CHUNK_BYTES) {
    expected_len = TELEMETRY_CHUNK_BYTES;
  }

  // Chunks must arrive in order, so a packet is never stitched together from
  // two different ones; chunk 0 always starts a fresh packet
  if (chunk_index == 0) {
    chunk_mask = 0;
  } else if (!(chunk_mask & (1U << (chunk_index - 1)))) {
    chunk_mask = 0;
    can_drop_count++;
    return;
  }

  if (header.DLC != expected_len) {
    chunk_mask = 0;
    can_drop_count++;
    return;
  }

  memcpy(&assembly[offset], payload, expected_len);
  chunk_mask |= 1U << chunk_index;

  if (chunk_mask == ALL_CHUNKS_MASK) {
    chunk_mask = 0;
    if (assembly[0] != TELEMETRY_PACKET_HEADER) {
      can_drop_count++;
      return;
    }
    memcpy(&latest_pkt, assembly, sizeof(latest_pkt));
    latest_is_new = true;
    can_packet_count++;
  }
}
