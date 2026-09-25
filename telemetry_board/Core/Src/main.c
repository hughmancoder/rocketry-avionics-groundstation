/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2026 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "LoRa.h"
#include "maxm10s_gps_driver.h"
#include "stm32l4xx_hal_can.h"
#include "telemetry_mock.h"
#include "telemetry_packet.h"
#include <stdio.h>
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

// Set to 1 to enable LoRa transceiver, or 0 to disable
#define ENABLE_LORA 1
// Set to 1 to turn board into Rocket Mock Transmitter, or 0 for Ground Station
#define MODE_MOCK_TRANSMITTER 1

#define GPS_FIX_TIMEOUT_MS 5000
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
CAN_HandleTypeDef hcan1;

I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
UART_HandleTypeDef huart2;

#if ENABLE_LORA
LoRa myLoRa;
uint8_t LoRa_stat = 0;
uint8_t ID = 0;

uint8_t rx_buffer[128];
uint8_t rx_bytes = 0;
int rx_rssi = 0;

TelemetryPacket tx_pkt;
#endif

static MAX_M10S_Data_t gps_data;
static uint32_t last_fix_ms = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_CAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */
static void MX_USART2_UART_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  MX_USART2_UART_Init(); // Console printf output (ST-Link / VCP)

  MAX_M10S_Init();
  last_fix_ms = HAL_GetTick();

#if ENABLE_LORA
  printf("\r\n=========================================\r\n");
  printf(" Ground Station Telemetry Receiver (LoRa)\r\n");
  printf("=========================================\r\n");

  myLoRa = newLoRa();

  myLoRa.CS_port = CS_GPIO_Port;
  myLoRa.CS_pin = CS_Pin;
  myLoRa.hSPIx = &hspi1;

  // Ensure CS is HIGH (idle) and give the SX1276 time to exit power-on reset
  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
  HAL_Delay(50);

  // Map DIO0 to PB0 (RX_DONE) based on the schematic
  myLoRa.DIO0_port = RX_Done_GPIO_Port;
  myLoRa.DIO0_pin = RX_Done_Pin;

  // Assign dummy reset pins to prevent null-pointer crashes (PA15 is open)
  myLoRa.reset_port = GPIOA;
  myLoRa.reset_pin = GPIO_PIN_15;

  // NOTE: schematic says PA_BOOST intended to run at +17dBm @ 915MHz
  myLoRa.frequency = 915;
  myLoRa.power = POWER_17db;

  // Initialize and check status
  uint16_t init_res = LoRa_init(&myLoRa);
  ID = LoRa_read(&myLoRa, RegVersion);

  if (init_res == LORA_OK) {
    LoRa_stat = 1;
    // Success - Turn LED on solid
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);

    printf("[LoRa] Init SUCCESS. Radio Version ID: 0x%02X\r\n", ID);
    printf("[LoRa] Configured: Frequency=915 MHz, Power=+17 dBm\r\n");

#if MODE_MOCK_TRANSMITTER
    printf("[LoRa] Operating Mode: MOCK TRANSMITTER (Sending 5 Hz "
           "telemetry)...\r\n\r\n");
#else
    printf("[LoRa] Operating Mode: GROUND STATION RECEIVER (Listening in "
           "RXCONTIN_MODE)...\r\n\r\n");
    // Put LoRa into continuous receive mode for Ground Station telemetry
    LoRa_gotoMode(&myLoRa, RXCONTIN_MODE);
#endif
  } else {
    LoRa_stat = 0;
    printf("[LoRa] Init FAILED (code: %d)! Version ID read: 0x%02X (Expected: "
           "0x12)\r\n",
           init_res, ID);
    printf("[LoRa] Check SPI wiring\r\n");
    printf("[LoRa] Continuing in diagnostic heartbeat mode...\r\n\r\n");
  }
#endif
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    // 1. Poll the GPS driver for new NMEA sentences
    MAX_M10S_Poll();

    // 2. Check if a complete sentence was parsed
    gps_data = MAX_M10S_GetData();
    if (gps_data.data_updated) {
      if (gps_data.fix_quality > 0) {
        last_fix_ms = HAL_GetTick();
      }
      printf("fix=%u sats=%u lat=%.6f lon=%.6f alt=%.1f\r\n",
             gps_data.fix_quality, gps_data.satellites, gps_data.latitude,
             gps_data.longitude, gps_data.altitude);
      printf("raw: %s\r\n", gps_last_sentence);
    } else if (HAL_GetTick() - last_fix_ms > GPS_FIX_TIMEOUT_MS) {
      printf("[GPS] no fix for over %d ms (waiting for satellites)\r\n",
             GPS_FIX_TIMEOUT_MS);
      last_fix_ms = HAL_GetTick();
    }

#if ENABLE_LORA
    if (LoRa_stat == 1) {
#if MODE_MOCK_TRANSMITTER
      // Generate mock sensor packet
      Telemetry_GenerateMockPacket(&tx_pkt);

      // Transmit telemetry struct over LoRa radio
      uint8_t tx_status =
          LoRa_transmit(&myLoRa, (uint8_t *)&tx_pkt, sizeof(tx_pkt), 1000);

      if (tx_status == 1) {
        printf("[TX] Packet #%lu Sent | Alt: %.1f m | Temp: %.2f C | AccZ: "
               "%.2fg\r\n",
               (unsigned long)tx_pkt.packet_num, tx_pkt.altitude_m,
               tx_pkt.temperature_C, tx_pkt.imu_accel[2]);
      } else {
        uint8_t opMode = LoRa_read(&myLoRa, RegOpMode);
        uint8_t irqFlags = LoRa_read(&myLoRa, RegIrqFlags);
        uint8_t ver = LoRa_read(&myLoRa, RegVersion);
        printf("[TX ERROR] Failed! OpMode=0x%02X | IrqFlags=0x%02X | "
               "Ver=0x%02X\r\n",
               opMode, irqFlags, ver);
      }

      HAL_Delay(200); // Send at 5 Hz (every 200 ms)
#else
      // Poll for incoming rocket telemetry packets
      rx_bytes = LoRa_receive(&myLoRa, rx_buffer, sizeof(rx_buffer));
      if (rx_bytes > sizeof(rx_buffer)) {
        rx_bytes = sizeof(rx_buffer);
      }
      if (rx_bytes > 0) {
        // Get RSSI of the received telemetry packet
        rx_rssi = LoRa_getRSSI(&myLoRa);

        // Deserialization: Check if packet matches structured TelemetryPacket
        if (rx_bytes == sizeof(TelemetryPacket) &&
            rx_buffer[0] == TELEMETRY_PACKET_HEADER) {
          TelemetryPacket pkt;
          memcpy(&pkt, rx_buffer, sizeof(TelemetryPacket));

          printf("---------------------------------------------------\r\n");
          printf("[RX TELEMETRY] Pkt #%lu | Time: %lu ms | RSSI: %d dBm\r\n",
                 (unsigned long)pkt.packet_num, (unsigned long)pkt.timestamp_ms,
                 rx_rssi);
          printf("   Altitude: %.1f m  | Temp: %.2f C\r\n", pkt.altitude_m,
                 pkt.temperature_C);
          printf("   ACC  (g) : X=%+.2f  Y=%+.2f  Z=%+.2f\r\n",
                 pkt.imu_accel[0], pkt.imu_accel[1], pkt.imu_accel[2]);
          printf("   GYRO(d/s): X=%+.2f  Y=%+.2f  Z=%+.2f\r\n", pkt.imu_gyro[0],
                 pkt.imu_gyro[1], pkt.imu_gyro[2]);
          printf("---------------------------------------------------\r\n\r\n");
        } else {
          // Raw packet fallback (HEX / Text string format)
          printf("[RX RAW] Received %d bytes | RSSI: %d dBm\r\n", rx_bytes,
                 rx_rssi);
          printf("   HEX: ");
          for (int i = 0; i < rx_bytes; i++) {
            printf("%02X ", rx_buffer[i]);
          }
          printf("\r\n   TXT: %.*s\r\n\r\n", rx_bytes, rx_buffer);
        }
      }
      HAL_Delay(10);
#endif
    } else {
      // Radio not initialized: wait gracefully without hammering the SPI bus
      HAL_Delay(50);
    }
#else
    // Reduce the delay slightly when polling
    HAL_Delay(50);
#endif
    /* USER CODE END 3 */
  }
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 18;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief CAN1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_CAN1_Init(void) {

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 9;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_12TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_3TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  /* USER CODE END CAN1_Init 2 */
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.Timing = 0x00E12573;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
    Error_Handler();
  }

  /** Configure Analogue filter
   */
  if (HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE) != HAL_OK) {
    Error_Handler();
  }

  /** Configure Digital filter
   */
  if (HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */
}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */
}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LORA_CS_GPIO_Port, LORA_CS_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, RX_Done_Pin | RX_Timeout_Pin | LED_HRT_BEAT_Pin,
                    GPIO_PIN_RESET);

  /*Configure GPIO pin : LORA_CS_Pin */
  GPIO_InitStruct.Pin = LORA_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LORA_CS_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : RX_Done_Pin RX_Timeout_Pin LED_HRT_BEAT_Pin */
  GPIO_InitStruct.Pin = RX_Done_Pin | RX_Timeout_Pin | LED_HRT_BEAT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */
  // Configure LoRa CS Pin (PA4) as output idle HIGH
  HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(CS_GPIO_Port, &GPIO_InitStruct);

  // Deselect on-board SPI Flash (PA5) so it doesn't contend on SPI1 MISO
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = GPIO_PIN_5;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  // PB0 (DIO0/RX_Done) and PB1 (DIO1/RX_Timeout) are outputs from the SX1276.
  // Reconfigure them as inputs so the MCU doesn't drive GND into the SX1276
  // output pins.
  GPIO_InitStruct.Pin = RX_Done_Pin | RX_Timeout_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
int _write(int file, char *ptr, int len) {
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  // PA2 -> USART2_TX, PA3 -> USART2_RX
  GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart2) != HAL_OK) {
    Error_Handler();
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state
   */
  __disable_irq();
  while (1) {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
     file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
