/**
 * ==============================================================================
 * @file    maxm10s_gps_driver.h
 * @brief   Driver for u-blox MAX-M10S GNSS Receiver on STM32L433.
 *
 * Supported Transports:
 *   - I2C (DDC): Polling mode via I2C1 (PA9 = SCL, PA10 = SDA)
 *                Address: 0x42 (0x84 write, 0x85 read)
 *   - UART:      Interrupt-driven mode via USART1 (PB6 = TX, PB7 = RX) @ 9600
 * baud
 *
 * Transport Selection:
 *   Change GPS_ACTIVE_TRANSPORT below to either GPS_TRANSPORT_I2C or
 *   GPS_TRANSPORT_UART.
 * ==============================================================================
 */
#ifndef MAXM10S_GPS_DRIVER_H
#define MAXM10S_GPS_DRIVER_H

#include <stdbool.h>
#include <stdint.h>

// Transport mode definitions
#define GPS_TRANSPORT_I2C 0
#define GPS_TRANSPORT_UART 1

// Select active transport here:
#ifndef GPS_ACTIVE_TRANSPORT
// #define GPS_ACTIVE_TRANSPORT GPS_TRANSPORT_I2C
#define GPS_ACTIVE_TRANSPORT GPS_TRANSPORT_UART
#endif

// GPS Data Structure
typedef struct {
  float latitude;
  float longitude;
  uint8_t fix_quality; // 0 = Invalid, 1 = GPS fix, 2 = DGPS fix
  uint8_t satellites;
  float altitude;
  bool data_updated;
} MAX_M10S_Data_t;

// Common API
void MAX_M10S_Init(void);
void MAX_M10S_Poll(void);
void MAX_M10S_ProcessChar(char c);
MAX_M10S_Data_t MAX_M10S_GetData(void);

// Diagnostic counters for live debugging in the Watch window
extern volatile uint32_t gps_rx_char_count;
extern volatile uint32_t gps_gga_count;
extern volatile uint32_t gps_error_count;
extern char gps_last_sentence[128];

#endif // MAXM10S_GPS_DRIVER_H