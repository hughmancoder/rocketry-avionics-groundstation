// ESP32 Arduino serial telemetry simulator.
// CSV format must match gui/src/pages/SettingsPage.tsx:
// time,Temp,pressure,altitude,accX,accY,accZ,angVelX,angVelY,angVelZ,lat,lon
#include <Arduino.h>

constexpr unsigned long SAMPLE_INTERVAL_MS = 250;

unsigned long lastSampleTime = 0;

float sampleValue(float minimum, float maximum) {
	return minimum + (static_cast<float>(esp_random()) / UINT32_MAX) * (maximum - minimum);
}

void sendSample() {
	const unsigned long timeMs = millis();
	const float temperature = sampleValue(20.0f, 35.0f);
	const float pressure = sampleValue(990.0f, 1025.0f);
	const float altitude = sampleValue(0.0f, 500.0f);
	const float accX = sampleValue(-2.0f, 2.0f);
	const float accY = sampleValue(-2.0f, 2.0f);
	const float accZ = sampleValue(8.0f, 10.0f);
	const float angVelX = sampleValue(-180.0f, 180.0f);
	const float angVelY = sampleValue(-180.0f, 180.0f);
	const float angVelZ = sampleValue(-180.0f, 180.0f);
	const float lat = sampleValue(-90.0f, 90.0f);
	const float lon = sampleValue(-180.0f, 180.0f);

	Serial.printf(
			"%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.6f,%.6f\n",
			timeMs,
			temperature,
			pressure,
			altitude,
			accX,
			accY,
			accZ,
			angVelX,
			angVelY,
			angVelZ,
			lat,
			lon);
}

void setup() {
	Serial.begin(115200);
	delay(500);
	randomSeed(esp_random());
}

void loop() {
	const unsigned long now = millis();
	if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
		lastSampleTime = now;
		sendSample();
	}
}
