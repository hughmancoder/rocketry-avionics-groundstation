# Simple Makefile wrapper for PlatformIO commands

# Default port for your ESP32 (CP2102 adapter)
PORT ?= /dev/cu.usbserial-0001


.PHONY: all build upload clean monitor list-ports

all: build

build:
	pio run

upload:
	pio run --target upload --upload-port $(PORT)

clean:
	pio run --target clean

monitor:
	pio device monitor --port $(PORT)

list-ports:
	pio device list

