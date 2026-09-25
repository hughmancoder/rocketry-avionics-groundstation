# GUI

<img alt="TypeScript" src="https://img.shields.io/badge/-TypeScript-007ACC?&logo=TypeScript&style=for-the-badge" />
<img alt="React" src="https://img.shields.io/badge/-React-61DAFB?&logo=React&style=for-the-badge" />

## Setup

install npm and node on your machine

```bash
cd gui # if not already in the gui directory
npm install
npm run dev
```

## Overview

- The GUI is built using React, TypeScript and tailwind.css.
- The GUI reads from a serial port from the [Web Serial API](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API) to get telemetry data from the groundstation.

- Important: check browser [compatibility](https://developer.mozilla.org/en-US/docs/Web/API/Web_Serial_API#browser_compatibility) for the Web Serial API

## Notes

List serial ports/

```javascript
# from browser console
navigator.serial.getPorts().then(ports => console.log(ports));
```

```bas
# from terminal
ls /dev/tty.*
```

## Simulate Serial Port

```bash
 socat -d -d pty,raw,echo=0 pty,raw,echo=0 # create serial port
 cd test
 python simulate_telemetry_serial.py
```


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


## Backlog

- [x] Real-time telemetry data
- [x] Graphs page
- [x] Settings page
- [x] Deploy to github pages
- [x] Teensy System Integration
- [ ] More robust disconnect mechanism (disconnect regardless of connection failure)
- [ ] Export flight data to CSV
- [ ] Database to save past flights
- [ ] Responsive UI (Gui can be used on phones/tablets)
