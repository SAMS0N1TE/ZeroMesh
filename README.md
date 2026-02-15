# ZeroMesh Serial

ZeroMesh is a serial interface for the Flipper Zero designed to communicate with Meshtastic nodes. It provides a direct link to a node via UART to monitor network status, manage a node roster, and send or receive messages.

Power Warning

Do not power a node from USB, battery, or any external supply while it is connected to the Flipper Zero 5V pin.

The 5V pin is an output. Multiple power sources on the same rail will back-feed and can damage the Flipper, the node’s regulator, or the USB port.

Use only one power source at a time. Disconnect the Flipper 5V before connecting USB or any other supply.


## Features

* **Roster Management**: Tracks discovered nodes, including Signal-to-Noise Ratio (SNR), Received Signal Strength Indicator (RSSI), and telemetry data like battery percentage and voltage.
* **Messaging**: Supports both broadcast and direct private messaging to specific nodes within the roster.
* **UI System**: A multi-page interface featuring message history, node lists, device statistics, signal information, and debug logs.
* **Notifications**: Configurable alerts using the Flipper Zero's vibration motor, LED, and internal speaker for incoming traffic.
* **Debug Console**: Real-time logging of raw protocol frames and internal application state.

## File Structure

* `zeromesh_serial_app.c`: Application entry point and main loop.
* `zeromesh_gui.c/h`: UI rendering, pagination, and input handling.
* `zeromesh_protocol.c/h`: Framing and Protobuf decoding for the Meshtastic serial protocol.
* `zeromesh_roster.c/h`: Node discovery and direct message routing.
* `zeromesh_uart.c/h`: Hardware serial configuration and stream buffering.
* `zeromesh_history.c/h`: Message storage and system logging.
* `zeromesh_notify.c/h`: Hardware notification triggers.

## Installation

1.  Copy the `zeromesh` folder into the `applications_user` directory of your Flipper Zero firmware source.
2.  Ensure the `lib/meshtastic_api` and `lib/nanopb` dependencies are present.
3.  Open a terminal in the project root and run:
    ```powershell
    ufbt launch
    ```

## Hardware Configuration

### Connection
Connect your Meshtastic node to the Flipper Zero GPIO pins:
* **TX**: Connect to Flipper RX (Pin 13/14 depending on UART selection).
* **RX**: Connect to Flipper TX (Pin 13/14 depending on UART selection).
* **GND**: Ensure a common ground between both devices.

### Node Settings
The Meshtastic node must be configured via the CLI or Mobile App:
* **Serial Module**: Enabled.
* **Serial Mode**: `PROTO`.
* **Baud Rate**: 115200.

## Usage

* **Navigation**: Use Left and Right buttons to switch between pages.
* **Messages**: Press OK on the Messages page to open the text input for broadcasting.
* **Roster**: Highlight a node and press OK to start a private chat, or hold OK to view full telemetry details.
* **Logs**: Press OK on the Logs page to pause the stream for review.
* **Settings**: Configure UART pins, baud rate, and notification preferences.

## License

This project is licensed under the GNU General Public License v3.0 (GPL-3.0).
