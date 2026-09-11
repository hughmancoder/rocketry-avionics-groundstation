import { Telemetry } from "./types";

export type DataSource = "srad" | "cots";

function numberOrNull(value: string): number | null {
  const parsed = Number(value);
  return Number.isFinite(parsed) ? parsed : null;
}

export function parseSradTelemetry(line: string): Telemetry | null {
  const parts = line.trim().split(",");
  if (parts.length < 12) {
    return null;
  }

  const values = parts.slice(0, 12).map(numberOrNull);
  if (values.some((value) => value === null)) {
    return null;
  }

  return {
    time: values[0]!,
    Temp: values[1]!,
    pressure: values[2]!,
    altitude: values[3]!,
    accX: values[4]!,
    accY: values[5]!,
    accZ: values[6]!,
    angVelX: values[7]!,
    angVelY: values[8]!,
    angVelZ: values[9]!,
    lat: values[10]!,
    lon: values[11]!,
  };
}

export function parseCotsGpsTelemetry(
  line: string,
  time: number,
): Telemetry | null {
  const parts = line.trim().replace(/^@\s*/, "").split(/\s+/);
  if (parts[0] !== "GPS_STAT") {
    return null;
  }

  const altitudeIndex = parts.indexOf("Alt");
  const latitudeIndex = parts.indexOf("lt");
  const longitudeIndex = parts.indexOf("ln");

  if (altitudeIndex < 0 || latitudeIndex < 0 || longitudeIndex < 0) {
    return null;
  }

  const altitudeFeet = numberOrNull(parts[altitudeIndex + 1]);
  const lat = numberOrNull(parts[latitudeIndex + 1]);
  const lon = numberOrNull(parts[longitudeIndex + 1]);

  if (altitudeFeet === null || lat === null || lon === null) {
    return null;
  }

  return {
    time,
    Temp: 0,
    pressure: 0,
    altitude: altitudeFeet * 0.3048,
    accX: 0,
    accY: 0,
    accZ: 0,
    angVelX: 0,
    angVelY: 0,
    angVelZ: 0,
    lat,
    lon,
  };
}
