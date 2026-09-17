# Adept Rocketry Ground Station

## Hardware

- Teensy 4.1
- LoRa Module

## Libraries

- RadioHead
- LiquidCrystal I2C (Frank de Brabander)

## Circuit schematic

![Groundstation circuit schematic](embedded/resources/GroundStation.png)


## Serial data packet structure

```c
struct TelemetryPacket {
  uint32_t time;                   // 4 bytes
  float altitude;                  // 4 bytes
  float bmpTemp;                   // 4 bytes
  float imuTemp;                   // 4 bytes
  float pressure;                  // 4 bytes
  float accX, accY, accZ;          // 12 bytes
  float angVelX, angVelY, angVelZ; // 12 bytes
  float lat, lon
} __attribute__((packed));         // Ensure no padding in the structure
```

Refer to [Telemetry.hpp](embedded/core/Telemetry.hpp) for the packet structure and shared utility functions.


## GUI

### Telemetry (Active)
![Telemetry Active](images/gui-images/telemetry-active.png)

### Telemetry (Inactive)
![Telemetry](images/gui-images/telemetry.png)

### GUI Graphs
![Graphs](images/gui-images/graphs.png)

### Settings
![Settings](images/gui-images/settings.png)

## About

Adept Rocketry Division's Groundstation. This repo consists of a web-based GUI  and the embedded system code which resides on the teensy microcontroller.

## Getting Started

The GUI can read telemetry from either a Web Serial connection or a WebSocket connection. The embedded system can send telemetry directly over serial, while the included serial-to-WebSocket bridge supports systems where the browser cannot access the virtual serial port.

The GUI supports two telemetry sources:

- **SRAD:** 12-field CSV telemetry containing time, temperature, pressure, altitude, acceleration, angular velocity, latitude, and longitude.
- **COTS Feather:** `GPS_STAT` packets containing latitude, longitude, and altitude. COTS altitude is converted from feet to meters for the GUI; unavailable sensor values are set to zero.

On the Settings page you can:

- Select SRAD or COTS Feather parsing.
- Select Serial or WebSocket as the connection type.
- Configure the WebSocket URL, which defaults to `ws://localhost:8765`.
- Change and apply the launch-site latitude and longitude.
- Export the collected telemetry history as a timestamped CSV file.

The map uses local satellite tiles and supports an interactive elevated flight-path overlay. Map tiles can be downloaded for a chosen center coordinate and radius with the offline map downloader. The downloader keeps its zoom levels hardcoded and writes tiles into `gui/public/maps`.

To forward a serial device over WebSocket, install the bridge dependencies and run:

```bash
pip install pyserial websockets
python3 tools/serial_websocket_bridge.py \
  --serial-port /dev/cu.usbserial-DK0JXP7Q \
  --baud 115200
```

Then select **WebSocket** in the GUI and use `ws://localhost:8765`.

### Default Configuration

GUI startup defaults are centralized in [gui/src/config.ts](gui/src/config.ts). Edit this file when deploying the ground station to a different launch site or telemetry setup:

```ts
export const DEFAULT_CONFIG = {
  launchSite: {
    latitude: -33.0305768,
    longitude: 136.3780941,
  },
  connection: {
    transport: "serial",
    websocketUrl: "ws://localhost:8765",
    baudRate: 115200,
  },
  dataSource: "cots",
};
```

Configuration fields:

- `launchSite.latitude` and `launchSite.longitude` set the initial map center and launch-site form values. Coordinates use decimal degrees.
- `connection.transport` selects the initial `serial` or `websocket` connection mode.
- `connection.websocketUrl` sets the initial WebSocket address when WebSocket mode is selected.
- `connection.baudRate` sets the Web Serial baud rate.
- `dataSource` selects the initial parser: `srad` or `cots`.

These are startup defaults. Settings changed through the GUI are not written back to the config file and reset when the application reloads.

Information on setting up the GUI is available in its README. Embedded-system details are documented below.

### GUI

[Gui README](gui/README.md)


```bash
# deploy manually 
cd gui
npm install gh-pages --save-dev
npm run deploy


## Features

- [x] Real-time telemetry data
- [x] Graphs page
- [x] Settings page
- [x] Deploy to github pages
- [x] Teensy System Integration
- [x] Offline satellite map with flight-path visualization
- [x] SRAD and COTS Feather telemetry sources
- [x] Web Serial and WebSocket telemetry connections
- [x] CSV flight-data export
- [ ] More robust disconnect mechanism (disconnect regardless of connection failure)
- [ ] Database to save past flights
- [ ] Responsive UI (Gui can be used on phones/tablets)
