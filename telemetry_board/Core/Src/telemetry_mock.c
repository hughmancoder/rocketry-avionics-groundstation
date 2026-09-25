#include "telemetry_mock.h"
#include "stm32l4xx_hal.h"
#include <math.h>

static uint32_t pkt_count = 0;

void Telemetry_GenerateMockPacket(TelemetryPacket *pkt) {
  pkt_count++;
  uint32_t now = HAL_GetTick();

  pkt->header = TELEMETRY_PACKET_HEADER;
  pkt->packet_num = pkt_count;
  pkt->timestamp_ms = now;

  // Simulated sensor readings (varying slightly over time)
  float time_sec = now / 1000.0f;
  pkt->temperature_C = 24.5f + (float)sin(time_sec * 0.1f) * 2.0f;
  pkt->imu_accel[0] = 0.05f + (float)sin(time_sec) * 0.2f;
  pkt->imu_accel[1] = -0.02f + (float)cos(time_sec) * 0.15f;
  pkt->imu_accel[2] = 1.0f + (float)sin(time_sec * 0.5f) * 0.05f; // ~1g gravity
  pkt->imu_gyro[0] = (float)sin(time_sec * 2.0f) * 5.0f;
  pkt->imu_gyro[1] = (float)cos(time_sec * 2.0f) * 4.0f;
  pkt->imu_gyro[2] = (float)sin(time_sec * 0.5f) * 1.5f;
  pkt->altitude_m = 150.0f + time_sec * 5.0f; // Mock ascending rocket altitude
  pkt->checksum = 0xFFFF;                     // Placeholder checksum
}
