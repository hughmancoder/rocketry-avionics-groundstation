"""Forward serial lines unchanged to every connected WebSocket client."""

import argparse
import asyncio
import threading
from typing import Any

import serial
import websockets


connected_clients: set[Any] = set()


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Forward serial data unchanged over WebSocket."
    )
    parser.add_argument(
        "--serial-port",
        required=False,
        help="Serial device example: /dev/cu.usbserial-DK0JXP7Q or COM3.",
        default="/tmp/virtual-serial-1"
    )
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--websocket-port", type=int, default=8765)
    return parser.parse_args()


def forward_serial_data(
    serial_port: str,
    baud: int,
    loop: asyncio.AbstractEventLoop,
) -> None:
    try:
        with serial.Serial(serial_port, baudrate=baud, timeout=1) as connection:
            print(f"Reading {serial_port} at {baud} baud")
            while True:
                raw_line = connection.readline()
                if not raw_line:
                    continue

                # The GUI expects text packets. Preserve every serial byte that
                # can be represented as ASCII, including the original newline.
                message = raw_line.decode("ascii", errors="replace")
                loop.call_soon_threadsafe(schedule_broadcast, loop, message)
    except serial.SerialException as error:
        loop.call_soon_threadsafe(
            print,
            f"Serial connection stopped: {error}",
        )


def schedule_broadcast(loop: asyncio.AbstractEventLoop, message: str) -> None:
    loop.create_task(broadcast(message))


async def broadcast(message: str) -> None:
    if not connected_clients:
        return

    results = await asyncio.gather(
        *(client.send(message) for client in connected_clients),
        return_exceptions=True,
    )
    disconnected = {
        client
        for client, result in zip(connected_clients, results)
        if isinstance(result, Exception)
    }
    connected_clients.difference_update(disconnected)


async def handle_client(websocket: Any, *_args: Any) -> None:
    connected_clients.add(websocket)
    remote = getattr(websocket, "remote_address", "unknown client")
    print(f"WebSocket client connected: {remote}")
    try:
        await websocket.wait_closed()
    finally:
        connected_clients.discard(websocket)
        print(f"WebSocket client disconnected: {remote}")


async def run(arguments: argparse.Namespace) -> None:
    loop = asyncio.get_running_loop()
    serial_thread = threading.Thread(
        target=forward_serial_data,
        args=(arguments.serial_port, arguments.baud, loop),
        daemon=True,
    )
    serial_thread.start()

    async with websockets.serve(
        handle_client,
        arguments.host,
        arguments.websocket_port,
    ):
        print(
            f"WebSocket server listening at "
            f"ws://{arguments.host}:{arguments.websocket_port}"
        )
        await asyncio.Future()


def main() -> None:
    arguments = parse_arguments()
    try:
        asyncio.run(run(arguments))
    except KeyboardInterrupt:
        print("\nBridge stopped.")


if __name__ == "__main__":
    main()
