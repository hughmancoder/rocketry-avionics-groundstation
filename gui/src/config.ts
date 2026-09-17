import type { DataSource } from "./serialParsers";

export const DEFAULT_CONFIG = {
  launchSite: {
    latitude: -34.429494,
    longitude: 139.600430,
  },
  connection: {
    transport: "serial" as const,
    websocketUrl: "ws://localhost:8765",
    baudRate: 115200,
  },
  dataSource: "cots" as DataSource,
};
