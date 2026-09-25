/**
 * ==============================================================================
 * @file    maxm10s_gps_driver.c
 * @brief   Driver implementation for u-blox MAX-M10S GNSS module.
 *
 * Supported Communication Transports (set in maxm10s_gps_driver.h):
 *
 * 1. I2C Polling Mode (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_I2C):
 *    - Uses STM32 I2C1 peripheral (PA9 = SCL, PA10 = SDA)
 *    - 7-bit module address: 0x42 (shifted 8-bit write: 0x84, read: 0x85)
 *    - Polls registers 0xFD/0xFE to query available data length
 *    - Reads available bytes from data stream register 0xFF
 *    - Called in main loop via MAX_M10S_Poll()
 *
 * 2. UART Interrupt Mode (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_UART):
 *    - Uses STM32 USART1 peripheral (PB6 = TX, PB7 = RX @ 9600 baud)
 *    - Characters received asynchronously byte-by-byte via HAL_UART_Receive_IT
 *    - Background reception in HAL_UART_RxCpltCallback
 *    - Recovers from UART framing/overrun errors in HAL_UART_ErrorCallback
 *
 * Both modes pass incoming characters to MAX_M10S_ProcessChar() to assemble
 * and parse $GNGGA / $GPGGA NMEA sentences into MAX_M10S_Data_t.
 * ==============================================================================
 */

#include "maxm10s_gps_driver.h"
#include "main.h"
#include "stm32l4xx_hal.h"
#include <stdlib.h>
#include <string.h>

#define NMEA_MAX_LENGTH 128

static char nmea_buffer[NMEA_MAX_LENGTH];
static uint8_t buffer_index = 0;
static MAX_M10S_Data_t gps_data = {0};

// Diagnostic counters visible in the Watch window
volatile uint32_t gps_rx_char_count = 0;
volatile uint32_t gps_gga_count = 0;
volatile uint32_t gps_error_count = 0;
char gps_last_sentence[NMEA_MAX_LENGTH] = {0};

#if (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_I2C)
/* ==============================================================================
 * I2C Transport Configuration
 * ==============================================================================
 */
#define MAXM10S_I2C_ADDR (0x42 << 1) // 0x84

extern I2C_HandleTypeDef hi2c1;

#elif (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_UART)
/* ==============================================================================
 * UART Transport Configuration
 * ==============================================================================
 */
extern UART_HandleTypeDef huart1;
static uint8_t rx_byte;

#endif

// Helper function to extract comma-delimited fields, correctly handling empty
// fields (,,)
static const char *get_field(const char *p, char *out, int max_len) {
  int i = 0;
  while (*p && *p != ',' && *p != '*' && *p != '\r' && *p != '\n' &&
         i < max_len - 1) {
    out[i++] = *p++;
  }
  out[i] = '\0';
  if (*p == ',') {
    p++;
  }
  return p;
}

// Helper function to convert NMEA coordinate format (DDMM.MMMMM) to Decimal
// Degrees
static float ConvertToDecimalDegrees(const char *nmea_coord, char direction) {
  if (nmea_coord == NULL || *nmea_coord == '\0')
    return 0.0f;

  char *dot = strchr(nmea_coord, '.');
  if (!dot)
    return 0.0f;

  // Degrees are the digits before minutes (minutes are always 2 digits before
  // dot)
  int deg_len = (dot - nmea_coord) - 2;
  if (deg_len <= 0)
    return 0.0f;

  char deg_str[5] = {0};
  if (deg_len >= (int)sizeof(deg_str))
    deg_len = sizeof(deg_str) - 1;

  strncpy(deg_str, nmea_coord, deg_len);
  float degrees = atof(deg_str);
  float minutes = atof(nmea_coord + deg_len);

  float decimal = degrees + (minutes / 60.0f);

  if (direction == 'S' || direction == 'W') {
    decimal = -decimal;
  }

  return decimal;
}

static void ParseGNGGA(char *sentence) {
  // Expected format:
  // $GNGGA,hhmmss.ss,llll.ll,a,yyyyy.yy,a,x,xx,x.x,x.x,M,x.x,M,x.x,xxxx*hh
  const char *p = sentence;
  char field[32];
  char lat_str[16] = {0};
  char lat_dir = 0;
  char lon_str[16] = {0};
  char lon_dir = 0;

  for (uint8_t field_idx = 0; *p && *p != '*' && *p != '\r' && *p != '\n';
       field_idx++) {
    p = get_field(p, field, sizeof(field));
    switch (field_idx) {
    case 2: // Latitude
      strncpy(lat_str, field, sizeof(lat_str) - 1);
      break;
    case 3: // N/S
      lat_dir = field[0];
      break;
    case 4: // Longitude
      strncpy(lon_str, field, sizeof(lon_str) - 1);
      break;
    case 5: // E/W
      lon_dir = field[0];
      break;
    case 6: // Fix Quality (0 = Invalid, 1 = GPS fix, 2 = DGPS fix)
      gps_data.fix_quality = (uint8_t)atoi(field);
      break;
    case 7: // Satellites tracked
      gps_data.satellites = (uint8_t)atoi(field);
      break;
    case 9: // Altitude (mean sea level)
      gps_data.altitude = (field[0] != '\0') ? (float)atof(field) : 0.0f;
      break;
    default:
      break;
    }
  }

  if (gps_data.fix_quality > 0 && lat_str[0] != '\0' && lon_str[0] != '\0') {
    gps_data.latitude = ConvertToDecimalDegrees(lat_str, lat_dir);
    gps_data.longitude = ConvertToDecimalDegrees(lon_str, lon_dir);
  } else {
    gps_data.latitude = 0.0f;
    gps_data.longitude = 0.0f;
  }
  gps_data.data_updated = true;
}

void MAX_M10S_Init(void) {
#if (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_I2C)
  // I2C1 is initialized in main.c by MX_I2C1_Init()
  // Give the GPS module a moment to boot
  HAL_Delay(100);

#elif (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_UART)
  // Ensure USART1 IRQ is enabled in NVIC
  HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  // Start interrupt reception for incoming NMEA stream from MAX-M10S
  HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  HAL_Delay(100);
#endif
}

void MAX_M10S_ProcessChar(char c) {
  gps_rx_char_count++;
  if (c == '$') {
    buffer_index = 0;
  }
  if (buffer_index < NMEA_MAX_LENGTH - 1) {
    nmea_buffer[buffer_index++] = c;
  }
  if (c == '\n') {
    nmea_buffer[buffer_index] = '\0';
    strncpy(gps_last_sentence, nmea_buffer, sizeof(gps_last_sentence) - 1);
    gps_last_sentence[sizeof(gps_last_sentence) - 1] = '\0';
    if (strncmp(nmea_buffer, "$GNGGA", 6) == 0 ||
        strncmp(nmea_buffer, "$GPGGA", 6) == 0) {
      gps_gga_count++;
      ParseGNGGA(nmea_buffer);
    }
  }
}

#if (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_I2C)
// Poll the I2C bus for waiting data - reads all available bytes in a single
// polling event
void MAX_M10S_Poll(void) {
  uint8_t len_bytes[2] = {0};
  uint16_t bytes_available = 0;

  // 1. Query the u-blox "Bytes Available" registers: 0xFD (High Byte) and 0xFE
  // (Low Byte)
  if (HAL_I2C_Mem_Read(&hi2c1, MAXM10S_I2C_ADDR, 0xFD, I2C_MEMADD_SIZE_8BIT,
                       len_bytes, 2, 50) != HAL_OK) {
    gps_error_count++;
    return;
  }

  bytes_available = (len_bytes[0] << 8) | len_bytes[1];

  // 0xFFFF or > 4096 indicates invalid/floating bus or uninitialized module
  if (bytes_available == 0 || bytes_available >= 4096) {
    return;
  }

  // 2. Read all waiting bytes in a single polling event
  uint8_t rx_buf[256];
  bool reached_empty = false;

  while (bytes_available > 0 && !reached_empty) {
    uint16_t read_len =
        (bytes_available > sizeof(rx_buf)) ? sizeof(rx_buf) : bytes_available;

    // Read the block from register 0xFF (Data Stream)
    if (HAL_I2C_Mem_Read(&hi2c1, MAXM10S_I2C_ADDR, 0xFF, I2C_MEMADD_SIZE_8BIT,
                         rx_buf, read_len, 100) != HAL_OK) {
      gps_error_count++;
      break;
    }

    for (uint16_t i = 0; i < read_len; i++) {
      // 0xFF is the default u-blox empty buffer response
      if (rx_buf[i] == 0xFF) {
        reached_empty = true;
        break;
      }
      MAX_M10S_ProcessChar((char)rx_buf[i]);
    }

    bytes_available -= read_len;
  }

  // 3. Continue draining until the I2C bus returns 0xFF before returning to
  // main loop delay
  while (!reached_empty) {
    uint8_t single_byte = 0xFF;
    if (HAL_I2C_Mem_Read(&hi2c1, MAXM10S_I2C_ADDR, 0xFF, I2C_MEMADD_SIZE_8BIT,
                         &single_byte, 1, 20) != HAL_OK ||
        single_byte == 0xFF) {
      break;
    }
    MAX_M10S_ProcessChar((char)single_byte);
  }
}

#elif (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_UART)
// UART mode: ensures receiver interrupt stays active and recovers if an error
// halted it
void MAX_M10S_Poll(void) {
  if (huart1.RxState == HAL_UART_STATE_READY) {
    __HAL_UART_CLEAR_OREFLAG(&huart1);
    __HAL_UART_CLEAR_NEFLAG(&huart1);
    __HAL_UART_CLEAR_FEFLAG(&huart1);
    __HAL_UART_CLEAR_PEFLAG(&huart1);
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    MAX_M10S_ProcessChar((char)rx_byte);
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart->Instance == USART1) {
    gps_error_count++;
    // Clear flags on overrun or framing error and re-arm reception
    __HAL_UART_CLEAR_OREFLAG(huart);
    __HAL_UART_CLEAR_NEFLAG(huart);
    __HAL_UART_CLEAR_FEFLAG(huart);
    __HAL_UART_CLEAR_PEFLAG(huart);
    huart->RxState = HAL_UART_STATE_READY;
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
  }
}
#endif

MAX_M10S_Data_t MAX_M10S_GetData(void) {
  MAX_M10S_Data_t data;
#if (GPS_ACTIVE_TRANSPORT == GPS_TRANSPORT_UART)
  __disable_irq();
  data = gps_data;
  gps_data.data_updated = false;
  __enable_irq();
#else
  data = gps_data;
  gps_data.data_updated = false;
#endif
  return data;
}