#!/usr/bin/env python3
"""
Serial port fan-out broker for macOS.

Reads from one physical serial port and republishes the data stream to
N virtual serial ports (pseudo-terminals), so multiple pyserial scripts
can each open their own port and read the exact same data simultaneously.

Usage:
    1. Set REAL_PORT below to your actual device (run `ls /dev/tty.*` to find it).
    2. Run: python3 serial_broker.py
    3. It prints paths like /tmp/virtual-serial-0, /tmp/virtual-serial-1, ...
    4. Point each of your 3 consumer scripts at one of those paths, e.g.:
           serial.Serial("/tmp/virtual-serial-0", 115200)

Requires: pip install pyserial
"""

import os
import pty
import time
import serial

REAL_PORT = "/dev/tty.usbserial-0001"  # <-- change to your real device
BAUDRATE = 115200
NUM_CONSUMERS = 3
SYMLINK_DIR = "/tmp"
DEBUG = True  # <-- flip to False to quiet it down


def debug(*args):
    if DEBUG:
        print("[DEBUG]", *args)


def create_virtual_port(index):
    master_fd, slave_fd = pty.openpty()
    slave_name = os.ttyname(slave_fd)
    symlink_path = os.path.join(SYMLINK_DIR, f"virtual-serial-{index}")
    if os.path.islink(symlink_path) or os.path.exists(symlink_path):
        os.remove(symlink_path)
    os.symlink(slave_name, symlink_path)
    print(f"Consumer {index}: connect pyserial to {symlink_path}")
    return master_fd


def main():
    print(f"Opening real port {REAL_PORT} @ {BAUDRATE} baud")
    real = serial.Serial(REAL_PORT, BAUDRATE, timeout=1)
    master_fds = [create_virtual_port(i) for i in range(NUM_CONSUMERS)]

    print("Broker running. Waiting for data... (Ctrl+C to stop)")
    try:
        while True:
            data = real.read(real.in_waiting or 1)
            if data:
                debug(f"read {len(data)} bytes: {data!r}")
                for fd in master_fds:
                    try:
                        os.write(fd, data)
                    except OSError as e:
                        debug(f"write to fd {fd} failed: {e}")
            else:
                time.sleep(0.01)
    except KeyboardInterrupt:
        print("\nStopping broker.")
    finally:
        real.close()
        for fd in master_fds:
            try:
                os.close(fd)
            except OSError:
                pass


if __name__ == "__main__":
    main()