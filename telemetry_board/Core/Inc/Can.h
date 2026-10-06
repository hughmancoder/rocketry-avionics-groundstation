#ifndef TELEMETRY_CAN_H
#define TELEMETRY_CAN_H

#include "main.h"
#include "telemetry_packet.h"
#include <stdbool.h>

// 1 = self-test without the flight computer: CAN runs in silent loopback (bus
// pins not driven) and the main loop sends a mock flight computer
// TelemetryPacket through the real chunked path (IDs 0xAA..), which is then
// reassembled and transmitted over LoRa. Set to 0 for the real flight computer.
#define CAN_LOOPBACK_TEST 1

HAL_StatusTypeDef Telemetry_CAN_Init(void);

/*
 * Copies the most recent complete TelemetryPacket received over CAN into
 * *out (all zero if none yet). Returns true if it arrived since the last call.
 */
bool Telemetry_CAN_GetLatest(TelemetryPacket *out);

#if CAN_LOOPBACK_TEST
// Sends a mock flight computer TelemetryPacket as chunks on IDs 0xAA..,
// exactly as the flight computer should
HAL_StatusTypeDef Telemetry_CAN_SendMockPacket(void);
#endif

// Diagnostic counters
extern volatile uint32_t can_frame_count;  // CAN frames accepted
extern volatile uint32_t can_packet_count; // complete packets reassembled
extern volatile uint32_t can_drop_count;   // frames/packets discarded

#endif
