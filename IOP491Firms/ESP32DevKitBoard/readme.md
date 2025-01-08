# ESP-NOW Fall Detection System

This project implements a fall detection system using ESP-NOW on ESP32. The system listens for signals indicating either a detected fall or a safe condition, then triggers a buzzer alarm or stops it accordingly.

## Features
- **ESP-NOW Communication:** Peer-to-peer communication without requiring Wi-Fi infrastructure.
- **Fall Detection:** Processes incoming signals to detect fall events.
- **Buzzer Alarm:** Activates a buzzer with a rising and falling tone when a fall is detected and deactivates it when a safe signal is received.

## Requirements
- ESP32 board
- Buzzer connected to GPIO pin 18
- Arduino IDE or PlatformIO
- ESP-NOW library

## Wiring
- Connect the positive terminal of the buzzer to GPIO pin 18.
- Connect the negative terminal of the buzzer to GND.

## Code Overview

### Global Variables
- `extractedFallSignalContent` and `extractedSafeSignalContent`: Stores the extracted content from incoming signals.
- `isFallDetected`: Indicates if a fall has been detected.
- `fallStartTime`: Records the time when a fall was detected.

### Key Functions
- **`extractFallSignalContent` and `extractSafeSignalContent`**: Extracts content from incoming messages to identify fall or safe signals.
- **`triggerBuzzer` and `risefall`**: Manages buzzer activation with a rising and falling tone pattern.
- **`onReceive`**: Callback to process incoming ESP-NOW messages and determine actions based on signal content.

### Main Workflow
1. Initializes ESP-NOW and Wi-Fi in `setup()`.
2. Continuously checks in `loop()` whether the buzzer has been active for 30 seconds and turns it off if so.
3. Processes incoming signals using the `onReceive` method of the `ESP_NOW_Peer_Class`.