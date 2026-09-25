#include "telemetry_packet.h"
#include <Arduino.h>
#include <LoRa.h>
#include <SPI.h>

// Define ESP32 pins used for LoRa module (inAir9B / SX1276)
#define LORA_SCK 18
#define LORA_MISO 19
#define LORA_MOSI 23
#define LORA_CS 5
#define LORA_RST 14
#define LORA_IRQ 4 // DIO0

// Built-in LED on most ESP32 dev boards
#define LED_PIN 2

// Use 915MHz
#define LORA_FREQUENCY 915E6

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ; // Wait for serial port

  Serial.println("\n[ESP32 Ground Station] Booting up...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Setup LoRa pins
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  // Initialize LoRa
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("[DEBUG] Starting LoRa failed! Check wiring.");
    while (1) {
      delay(1000); // Halt if LoRa fails to initialize
    }
  }

  // Match the STM32 transmitter power setting (11dBm) to save power and prevent
  // potential voltage drops
  // TODO: match with telemetry_board (17dBm)
  LoRa.setTxPower(17);

  // Note: These settings MUST match the transmitter (telemetry_board)
  LoRa.setSpreadingFactor(7);     // SF_7
  LoRa.setSignalBandwidth(125E3); // BW_125KHz
  LoRa.setCodingRate4(5);         // CR_4_5
  LoRa.setPreambleLength(8);      // preamble = 8

  // Match STM32 transmitter: CRC is NOT enabled on STM32 by default
  LoRa.disableCrc();

  Serial.println("[ESP32 Ground Station] LoRa Initialized successfully.");

  // DUMP REGISTERS FOR HARDWARE DEBUGGING
  Serial.println("[DEBUG] Dumping SX1276 Radio Registers:");
  LoRa.dumpRegisters(Serial);
  Serial.println("---------------------------------------");
}

void loop() {
  // Check if a packet has been received
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.printf("\n[WE GOT A PACKET!] Size: %d bytes, RSSI: %d dBm, SNR: %.1f dB\n",
                  packetSize, LoRa.packetRssi(), LoRa.packetSnr());

    // Read all bytes into a raw buffer
    uint8_t buffer[256];
    int len = 0;
    while (LoRa.available() && len < sizeof(buffer)) {
      buffer[len++] = LoRa.read();
    }

    // Print raw HEX dump
    Serial.print("   RAW HEX: ");
    for (int i = 0; i < len; i++) {
      Serial.printf("%02X ", buffer[i]);
    }
    Serial.println();

    // Check if packet matches structured TelemetryPacket
    if (len >= sizeof(TelemetryPacket)) {
      TelemetryPacket packet;
      memcpy(&packet, buffer, sizeof(TelemetryPacket));

      if (packet.header == TELEMETRY_PACKET_HEADER) {
        digitalWrite(LED_PIN, HIGH);

        // Output CSV for the GUI
        Serial.printf(
            "[CSV] %lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
            (unsigned long)packet.timestamp_ms, packet.temperature_C,
            packet.temperature_C, packet.pressure_hPa, packet.altitude_m,
            packet.imu_accel[0], packet.imu_accel[1], packet.imu_accel[2],
            packet.imu_gyro[0], packet.imu_gyro[1], packet.imu_gyro[2]);

        digitalWrite(LED_PIN, LOW);
      } else {
        Serial.printf(
            "   [NOTE] Header byte is 0x%02X (expected 0x%02X)\n",
            packet.header, TELEMETRY_PACKET_HEADER);
      }
    }
  }
}
