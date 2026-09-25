# Simple Makefile wrapper for PlatformIO commands

.PHONY: all build upload clean monitor

all: build

build:
	pio run

upload:
	pio run --target upload

clean:
	pio run --target clean

monitor:
	pio device monitor
