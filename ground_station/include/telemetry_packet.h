#ifndef __TELEMETRY_PACKET_H
#define __TELEMETRY_PACKET_H

#include <stdint.h>

#define TELEMETRY_PACKET_HEADER 0xAA
#define TELEMETRY_NUM_CHUNKS 6U
#define TELEMETRY_CHUNK_BYTES 8U

/**
 * @brief Telemetry packet structure for Rocket <-> Ground Station transmission.
 *        Uses __attribute__((packed)) to guarantee uniform byte layout across
 * compiler targets.
 */
typedef struct __attribute__((packed)) {
  uint8_t header;        // Magic byte (0xAA) to identify valid telemetry packet
  uint32_t packet_num;   // Incremental packet counter
  uint32_t timestamp_ms; // System time in milliseconds
  int8_t h3lis_accel[3];
  float imu_accel[3];
  float imu_gyro[3];
  float temperature_C;
  float pressure_hPa;
  float altitude_m;
  uint16_t checksum; // Optional packet checksum
} TelemetryPacket;

#define TELEMETRY_PACKET_SIZE (sizeof(TelemetryPacket))

#endif /* __TELEMETRY_PACKET_H */
