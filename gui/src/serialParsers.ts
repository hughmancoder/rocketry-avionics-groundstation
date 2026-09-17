import { DATA_COLUMNS, Telemetry } from "./types";

export type DataSource = "srad" | "cots" | "replay";

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

export function parseTelemetryCsv(csv: string): Telemetry[] {
  const lines = csv.split(/\r?\n/).filter((line) => line.trim().length > 0);
  if (lines.length === 0) {
    return [];
  }

  const header = lines[0].split(",").map((column) => column.trim());
  const columns = DATA_COLUMNS.map(({ key }) => key);
  const columnIndexes = columns.map((column) => header.indexOf(column));
  const hasHeader = columnIndexes.every((index) => index >= 0);
  const dataLines = hasHeader ? lines.slice(1) : lines;

  return dataLines
    .map((line) => {
      const values = line.split(",");
      const packet = columns.map((column, index) => ({
        column,
        value: numberOrNull(values[hasHeader ? columnIndexes[index] : index] ?? ""),
      }));

      if (packet.some(({ value }) => value === null)) {
        return null;
      }

      return Object.fromEntries(
        packet.map(({ column, value }) => [column, value]),
      ) as Telemetry;
    })
    .filter((packet): packet is Telemetry => packet !== null);
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
