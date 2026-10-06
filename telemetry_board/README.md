# Telemetry Board

`STM32L433CCUx` based firmware for the Telemetry Board. The board interfaces with an SX1276/7/8/9 LoRa transceiver to transmit telemetry data.

## Getting Started

To modify hardware peripherals (like pins, clocks, or SPI settings), you will use **STM32CubeMX**:

1. Open the [`LoRa_new.ioc`](LoRa_new.ioc) file in **STM32CubeMX**.
2. Make your desired hardware changes in the Pinout & Configuration tab.
3. Click **Generate Code** in the top right corner.
4. STM32CubeMX will automatically update the files in `Core/Src/` and `Core/Inc/`.

> [!WARNING]
> Only write your custom C code between the `/* USER CODE BEGIN ... */` and `/* USER CODE END ... */` comments. Any code placed outside these blocks will be permanently deleted by STM32CubeMX when you click Generate Code!

## Build and Upload

### Building outside of STM32 IDE

**[platformio.ini](platformio.ini)**: Declares MCU targets (`stm32l433ccu6`), frameworks, and build options. 

```bash
# Build the project
make build      # or: pio run

# Upload firmware to board
make upload     # or: pio run -t upload

# Monitor serial output / logging
make monitor    # or: pio device monitor

# Clean build artifacts
make clean      # or: pio run -t clean
```

## Operating Modes & Mock Telemetry

Toggle between Rocket Transmitter and Ground Station Receiver in [`Core/Src/main.c`](Core/Src/main.c):

```c
#define MODE_MOCK_TRANSMITTER 1 // 1 = Mock Rocket TX (5 Hz), 0 = Ground Station RX
```

* **Transmitter (`1`)**: Sends simulated IMU, temperature, and altitude packets (`TelemetryPacket`) over LoRa.
* **Receiver (`0`)**: Runs in continuous receive mode (`RXCONTIN_MODE`), deserializes incoming telemetry packets, and prints parsed values over UART 

## Known Schamtic Issues

**LoRa Transceiver**

The `SX1276IMLTRT` chip is fully capable of both transmitting and receiving.

Reciving RFI_LF and RFI_HF ports are left unconnected, meaning incoming RF signals have no physical path into the chip's receiver. Therefore the board is not capable of receive?

**GPS ports**

CU_UART_TX (PB6)
MCU_UART_R (PB7)

**SPI MOSI/MISO GPS are swapped **

**PA12, PA11 CAN TX and CAN RX ports are wired wrong**
On the STM32L433 driver:

PA11is hardwired internally to CAN1_RX
PA12is hardwired internally to CAN1_TX


> **Antenna Safety Warning** 
Always ensure your LoRa SMA antenna is securely connected to the board before uncommenting the code and powering the system. Transmitting at +17dBm without an antenna connected will reflect power back into the SX1276 chip and can permanently destroy its internal amplifier.

