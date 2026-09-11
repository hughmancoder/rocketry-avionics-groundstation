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
  float lat, log 
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

### Settings (Placeholder Mock Data)
![Settings](images/gui-images/settings.png)

## About

Adept Rocketry Division's Groundstation. This repo consists of a web-based GUI  and the embedded system code which resides on the teensy microcontroller.

## Getting Started

The GUI reads from a serial port to get telemetry data from the rocket. The embedded system code sends telemetry data over the serial port. Information on how to set up the GUI is available in its README. Embedded-system details are documented below.

### GUI

[Gui README](gui/README.md)


## Deployment

The gui is deployed to github pages so we don't have to run it from a terminal. It can be run manually by following the instructions in the gui README.

Deployment script in deployment.yml

```bash
# deploy manually 
cd gui
npm install gh-pages --save-dev
npm run deploy

# deploys automatically with github actions with git commits
git add .
git commit -m "Deploy to GitHub Pages"
git push origin main
```

## Features

- [x] Real-time telemetry data
- [x] Graphs page
- [x] Settings page
- [x] Deploy to github pages
- [x] Teensy System Integration
- [ ] More robust disconnect mechanism (disconnect regardless of connection failure)
- [ ] Export flight data to CSV
- [ ] Database to save past flights
- [ ] Responsive UI (Gui can be used on phones/tablets)
