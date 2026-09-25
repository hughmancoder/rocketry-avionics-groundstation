#include "Can.h"
#include "main.h"
#include "telemetry_packet.h"
#include <stdint.h>
#include <string.h>

extern CAN_HandleTypeDef hcan1;

static uint8_t rx_buffer[TELEMETRY_PACKET_SIZE];

void Telemetry_CAN_Init(void) {
  CAN_FilterTypeDef canfilterconfig;
  canfilterconfig.FilterActivation = CAN_FILTER_ENABLE;
  canfilterconfig.FilterBank = 0;
  canfilterconfig.FilterFIFOAssignment = CAN_RX_FIFO0;
  canfilterconfig.FilterIdHigh = 0xAA << 5;
  canfilterconfig.FilterIdLow = 0x0000;
  canfilterconfig.FilterMaskIdHigh = 0;
  canfilterconfig.FilterMaskIdLow = 0xFFF0;
  canfilterconfig.FilterMode = CAN_FILTERMODE_IDMASK;
  canfilterconfig.FilterScale = CAN_FILTERSCALE_32BIT;

  HAL_CAN_ConfigFilter(&hcan1, &canfilterconfig);
  HAL_CAN_Start(&hcan1);
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan) {
  uint8_t payload[8];
  CAN_RxHeaderTypeDef header;
  if (HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &header, payload) != HAL_OK) {
    return;
  }

  if (header.IDE != CAN_ID_STD ||
      header.StdId >= TELEMETRY_PACKET_HEADER + TELEMETRY_NUM_CHUNKS) {
    return;
  }

  uint8_t chunk_index = (uint8_t)(header.StdId - TELEMETRY_PACKET_HEADER);

  // Figure out how to find remaining bytes
  uint8_t offset = chunk_index * TELEMETRY_CHUNK_BYTES;
  memcpy(&rx_buffer[offset], payload, header.DLC);
}