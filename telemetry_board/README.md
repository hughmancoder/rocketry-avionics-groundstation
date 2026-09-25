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

## Known Schematic Issues

### 1. CAN Transceiver TX and RX Swapped (PA11 / PA12)
* **Issue**: On the STM32L433CCU6:
  * `PA11` is internally `CAN1_RX`
  * `PA12` is internally `CAN1_TX`
  * In the schematic (Sheet 1), `PA11` is wired to `MCU_CAN_TX` &rarr; `R7` (0&Omega;) &rarr; `TCVR_CAN_TX` (Pin 1 `D` of SN65HVD230 transceiver).
  * `PA12` is wired to `MCU_CAN_RX` &rarr; `R8` (0&Omega;) &rarr; `TCVR_CAN_RX` (Pin 4 `R` of SN65HVD230 transceiver).
  * Consequently, the MCU CAN RX pin is connected to the transceiver driver input, and the MCU CAN TX pin is connected to the receiver output.
* **Workaround for Assembled PCBs**: Remove 0&Omega; resistors `R7` and `R8`, and cross-connect the pads with bodge wires (`PA11` pad to `TCVR_CAN_RX` pad, `PA12` pad to `TCVR_CAN_TX` pad).
* **Fix for Next PCB Rev**: Swap traces on `PA11` and `PA12` at the MCU.

### 2. LoRa Transceiver Receive Path Disconnected (TX Only)
* **Issue**: The `SX1276IMLTRT` transceiver (Sheet 2) only has `PA_BOOST` (Pin 27) connected to the matching network and SMA connector `J2`. Receiver inputs `RFI_LF` (Pin 1) and `RFI_HF` (Pin 21) are left unconnected / tied to ground.
* **Impact**: Without an RF switch (SPDT) to route the antenna between `PA_BOOST` (TX) and `RFI_HF` (RX), the board cannot receive RF signals over the air. Operating Mode 0 (`RXCONTIN_MODE`) cannot receive packets.
* **Fix for Next PCB Rev**: Add an RF switch (e.g. SKY13317, PE4259) controlled by `RXTX/RF_MOD` (Pin 20) or an MCU GPIO to switch the SMA antenna between `PA_BOOST` and `RFI_HF`.

### 3. GPS (MAX-M10S) Pin Connections
* **Pins**:
  * UART: `PB6` (`USART1_TX`) &rarr; GPS Pin 3 (`RXD`), and `PB7` (`USART1_RX`) &rarr; GPS Pin 2 (`TXD`).
  * I2C: `PA9` (`I2C1_SCL`) &rarr; GPS Pin 17 (`SCL`), and `PA10` (`I2C1_SDA`) &rarr; GPS Pin 16 (`SDA`).
* **Clarification**: The GPS does not interface via SPI on this board; SPI is reserved for the LoRa transceiver and SPI Flash.

### 4. LoRa Interrupt Pins Configured as Outputs (PB0 / PB1)
* **Issue**: In `LoRa_new.ioc`, `PB0` (`DIO0`/`RX_Done`) and `PB1` (`DIO1`/`RX_Timeout`) were initially generated as `GPIO_Output`.
* **Fix**: In firmware (`Core/Src/main.c`), reconfigure `PB0` and `PB1` as `GPIO_MODE_INPUT` so the MCU does not drive ground into the SX1276 interrupt outputs.


> **Antenna Safety Warning** 
Always ensure your LoRa SMA antenna is securely connected to the board before uncommenting the code and powering the system. Transmitting at +17dBm without an antenna connected will reflect power back into the SX1276 chip and can permanently destroy its internal amplifier.

