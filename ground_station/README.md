# ESP32 Ground Station Firmware

Firmware for the ESP32-based Ground Station receiver and [inAIR9B Lora radio](https://modtronix.com/product/inair9b/).
Refer to the `makefile` for build commands.

---

## Wiring Guide

| inAir9B Pin | ESP32 Pin | Description |
| :--- | :--- | :--- |
| **3V3** | 3V3 | Main Power (DO NOT connect to 5V) |
| **VS** | 3V3 | RF Switch Power (CRITICAL: Must be 3.3V unless J1 is soldered) |
| **GND** | GND | Ground |
| **SCK** | GPIO 18 (VSPI) | SPI Clock |
| **MISO** | GPIO 19 (VSPI) | SPI MISO |
| **MOSI** | GPIO 23 (VSPI) | SPI MOSI |
| **CS / NSS**| GPIO 5 | Chip Select |
| **RST** | GPIO 14 | Reset |
| **DIO0** | GPIO 4 | Interrupt / RX Done |

---

## Flashing the ESP32

**Identify Connected Device Port**

```bash
pio device list
```


