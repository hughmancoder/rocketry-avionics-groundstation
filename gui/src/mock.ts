import { Telemetry } from "./types";

  
  export const startMockTelemetry = (
    onData: (packet: Telemetry) => void,
    interval = 1000
  ) => {
    const timer = setInterval(() => {
      const packet: Telemetry = {
        time: Date.now(),
        Temp: 20 + Math.random() * 10,
        pressure: 1013 + Math.random() * 20 - 10,
        altitude: Math.random() * 100,
        accX: Math.random() * 2 - 1,
        accY: Math.random() * 2 - 1,
        accZ: Math.random() * 2 - 1,
        angVelX: Math.random() * 2 - 1,
        angVelY: Math.random() * 2 - 1,
        angVelZ: Math.random() * 2 - 1,
        lat: Math.random() * 180 - 90,
        lon: Math.random() * 360 - 180
      };
      onData(packet);
    }, interval);
    return () => clearInterval(timer);
  };
  