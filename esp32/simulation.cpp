// ESP32 Arduino serial telemetry simulator.
// CSV format must match gui/src/pages/SettingsPage.tsx:
// time,Temp,pressure,altitude,accX,accY,accZ,angVelX,angVelY,angVelZ,lat,lon
#include <Arduino.h>
#include <math.h>

constexpr unsigned long SAMPLE_INTERVAL_MS = 250;
constexpr float MAP_CENTER_LAT = -30.664806f;
constexpr float MAP_CENTER_LON = 143.196306f;
constexpr float METERS_PER_DEGREE = 111111.0f;
constexpr float FLIGHT_DURATION_SECONDS = 78.0f;
constexpr float DOWNRANGE_METERS_PER_SECOND = 55.0f;

unsigned long lastSampleTime = 0;
unsigned long flightStartTime = 0;

float sampleValue(float minimum, float maximum) {
	return minimum + (static_cast<float>(esp_random()) / UINT32_MAX) * (maximum - minimum);
}

float interpolate(float startTime, float startAltitude, float endTime,
		float endAltitude, float elapsedSeconds) {
	const float progress = (elapsedSeconds - startTime) / (endTime - startTime);
	return startAltitude + progress * (endAltitude - startAltitude);
}

float flightAltitude(float elapsedSeconds) {
	if (elapsedSeconds <= 6.3f) {
		return interpolate(0.0f, 0.0f, 6.3f, 620.0f, elapsedSeconds);
	}
	if (elapsedSeconds <= 28.0f) {
		return interpolate(6.3f, 620.0f, 28.0f, 3320.0f, elapsedSeconds);
	}
	if (elapsedSeconds <= 42.0f) {
		return interpolate(28.0f, 3320.0f, 42.0f, 2300.0f, elapsedSeconds);
	}
	return interpolate(42.0f, 2300.0f, FLIGHT_DURATION_SECONDS, 0.0f,
			elapsedSeconds);
}

float elapsedFlightSeconds(unsigned long now) {
	const float elapsedSeconds = (now - flightStartTime) / 1000.0f;
	return fminf(elapsedSeconds, FLIGHT_DURATION_SECONDS);
}

float latitudeFromNorthMeters(float northMeters) {
	return MAP_CENTER_LAT + northMeters / METERS_PER_DEGREE;
}

float longitudeFromEastMeters(float eastMeters) {
	const float latitudeRadians = MAP_CENTER_LAT * (PI / 180.0f);
	return MAP_CENTER_LON + eastMeters /
			(METERS_PER_DEGREE * cosf(latitudeRadians));
}

void sendSample() {
	const unsigned long timeMs = millis();
	const float flightTime = elapsedFlightSeconds(timeMs);
	const float altitude = flightAltitude(flightTime);
	const float downrange = DOWNRANGE_METERS_PER_SECOND * flightTime;
	const float eastWindDrift = 35.0f * sinf(flightTime * 0.35f);
	const float northWindDrift = 20.0f * sinf(flightTime * 0.22f + 1.0f);
	const float east = 0.55f * downrange + eastWindDrift;
	const float north = 0.25f * downrange + northWindDrift;
	const float temperature = 25.0f - 0.0065f * altitude + sampleValue(-0.5f, 0.5f);
	const float pressure = 1013.25f * expf(-altitude / 8434.5f);
	const float accX = sampleValue(-2.0f, 2.0f);
	const float accY = sampleValue(-2.0f, 2.0f);
	const float accZ = sampleValue(8.0f, 10.0f);
	const float angVelX = sampleValue(-180.0f, 180.0f);
	const float angVelY = sampleValue(-180.0f, 180.0f);
	const float angVelZ = sampleValue(-180.0f, 180.0f);
	const float lat = latitudeFromNorthMeters(north);
	const float lon = longitudeFromEastMeters(east);

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
	flightStartTime = millis();
}

void loop() {
	const unsigned long now = millis();
	if (now - lastSampleTime >= SAMPLE_INTERVAL_MS) {
		lastSampleTime = now;
		sendSample();
	}
}
