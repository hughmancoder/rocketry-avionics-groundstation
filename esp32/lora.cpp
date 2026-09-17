#include <Arduino.h>
#include <LoRa.h>

// SX1276 connected to the ESP32 VSPI bus.
// Change these values if your wiring uses different GPIOs.
constexpr int LORA_SCK = 18;
constexpr int LORA_MISO = 19;
constexpr int LORA_MOSI = 23;
constexpr int LORA_SS = 5;
constexpr int LORA_RESET = 14;
constexpr int LORA_DIO0 = 26;

// The inAir9B must use the same frequency as the rocket transmitter.
// Select 915E6 for the 902-928 MHz version or 868E6 for the 863-870 MHz version.
constexpr long LORA_FREQUENCY = 915E6;

constexpr size_t TELEMETRY_FIELD_COUNT = 12;

bool isTelemetryCsv(const String& payload) {
	size_t fieldCount = 1;
	for (size_t index = 0; index < payload.length(); ++index) {
		if (payload[index] == ',') {
			++fieldCount;
		}
	}
	return fieldCount == TELEMETRY_FIELD_COUNT;
}

void forwardPacket() {
	const int packetSize = LoRa.parsePacket();
	if (packetSize <= 0) {
		return;
	}

	String payload;
	payload.reserve(packetSize);
	while (LoRa.available()) {
		payload += static_cast<char>(LoRa.read());
	}

	payload.trim();
	if (isTelemetryCsv(payload)) {
		Serial.println(payload);
	}

	LoRa.receive();
}

void setup() {
	Serial.begin(115200);
	while (!Serial) {
		delay(10);
	}

	SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_SS);
	LoRa.setPins(LORA_SS, LORA_RESET, LORA_DIO0);

	if (!LoRa.begin(LORA_FREQUENCY)) {
		Serial.println("LORA_INIT_ERROR");
		while (true) {
			delay(1000);
		}
	}

	LoRa.enableCrc();
	LoRa.receive();
}

void loop() {
	forwardPacket();
}
