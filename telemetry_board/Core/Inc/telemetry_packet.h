#ifndef __TELEMETRY_PACKET_H
#define __TELEMETRY_PACKET_H

#include <stdint.h>

#define TELEMETRY_PACKET_HEADER 0xAA

/**
 * @brief Telemetry packet structure for Rocket <-> Ground Station transmission.
 *        Uses __attribute__((packed)) to guarantee uniform byte layout across compiler targets.
 */
typedef struct __attribute__((packed)) {
    uint8_t  header;       // Magic byte (0xAA) to identify valid telemetry packet
    uint32_t packet_num;   // Incremental packet counter
    uint32_t timestamp_ms; // System time in milliseconds
    int8_t h3lis_accel[3];
    float imu_accel[3];
    float imu_gyro[3];
    float temperature_C;
    float pressure_hPa;
    float altitude_m;
    uint16_t checksum;     // Optional packet checksum
} TelemetryPacket;

#define TELEMETRY_PACKET_SIZE (sizeof(TelemetryPacket))

/*
 * CAN transport: a TelemetryPacket is sent as consecutive 8-byte chunks on
 * standard IDs TELEMETRY_PACKET_HEADER + chunk index (0xAA, 0xAB, ...), in
 * order. The last chunk carries the remaining bytes (DLC < 8).
 */
#define TELEMETRY_CHUNK_BYTES 8U
#define TELEMETRY_NUM_CHUNKS                                                   \
    ((TELEMETRY_PACKET_SIZE + TELEMETRY_CHUNK_BYTES - 1) / TELEMETRY_CHUNK_BYTES)

/*
 * LoRa downlink frame: the latest sensor packet received over CAN plus the
 * board's own GPS data.
 */
#define LORA_FRAME_HEADER 0xA5

#define LORA_FLAG_CAN_NEW 0x01 // sensors: new CAN packet since the last frame
#define LORA_FLAG_CAN_ANY 0x02 // sensors: at least one CAN packet received
#define LORA_FLAG_GPS_FIX 0x04 // gps_*: valid fix

typedef struct __attribute__((packed)) {
    uint8_t  header;       // LORA_FRAME_HEADER
    uint32_t frame_num;    // Incremental LoRa frame counter
    uint32_t timestamp_ms; // Telemetry board time in milliseconds
    uint8_t  flags;        // LORA_FLAG_*
    float    gps_lat;      // Decimal degrees (+N / -S), 0 without fix
    float    gps_lon;      // Decimal degrees (+E / -W), 0 without fix
    float    gps_alt_m;    // Altitude above mean sea level, 0 without fix
    uint8_t  gps_fix;      // GGA fix quality (0 = none)
    uint8_t  gps_sats;     // Satellites used
    TelemetryPacket sensors; // Latest CAN packet (all zero until one arrives)
} LoRaTelemetryFrame;

/*
 * Compact GPS-only LoRa packet (GPS_ONLY_TX mode): position + satellite
 * count, 10 bytes.
 * Coordinates are degrees * 1e7; both 0 = no fix. (The GPS driver parses to
 * float, so actual precision is ~1-2 m.)
 */
#define LORA_GPS_HEADER 0xA6

typedef struct __attribute__((packed)) {
    uint8_t header;  // LORA_GPS_HEADER
    int32_t lat_e7;  // Latitude  (+N / -S) in 1e-7 degrees
    int32_t lon_e7;  // Longitude (+E / -W) in 1e-7 degrees
    uint8_t sats;    // Satellites used in the solution (0 = none)
} LoRaGpsPacket;

#endif /* __TELEMETRY_PACKET_H */
