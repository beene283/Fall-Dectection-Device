# Fall Detection System with ESP32 and MPU6050

This project is an ESP32-based fall detection system that uses the MPU6050 accelerometer and gyroscope module. It detects sudden movements and possible falls, and sends alerts using ESP-NOW communication protocol. The system also includes a manual "safe signal" button to reset fall alerts.

---

## Features

- **Fall Detection**: Detects falls based on fluctuations in accelerometer data.
- **ESP-NOW Communication**: Broadcasts fall alerts to other ESP32 devices in the network.
- **Manual Reset**: A button is used to indicate the patient is safe, resetting the system.
- **Debounce Logic**: Handles button state changes to avoid false triggers.
- **Robust Fall Verification**: Implements time-based and threshold-based logic to minimize false positives.

---

## Hardware Requirements

- ESP32 board
- MPU6050 accelerometer and gyroscope module
- Push button
- Resistors for button debouncing (optional)
- Breadboard and jumper wires

---

## Software Requirements

- Arduino IDE
- ESP32 core for Arduino
- Libraries:
  - [ESP32_NOW](https://github.com/espressif/arduino-esp32)
  - [Adafruit MPU6050](https://github.com/adafruit/Adafruit_MPU6050)
  - [Adafruit Sensor](https://github.com/adafruit/Adafruit_Sensor)

---

## Pin Configuration

| Component         | ESP32 Pin |
|--------------------|-----------|
| Button            | GPIO 2    |
| MPU6050 (SDA)     | GPIO 21   |
| MPU6050 (SCL)     | GPIO 22   |

---

## Setup Instructions

1. **Install Dependencies**:
   - Install the required libraries using the Arduino Library Manager.

2. **Hardware Connections**:
   - Connect the MPU6050 module to the ESP32 using I2C (SDA to GPIO 21, SCL to GPIO 22).
   - Connect a push button to GPIO 2 with one terminal connected to GND.

3. **Configure Parameters**:
   - Update the Wi-Fi channel in the code (`ESPNOW_WIFI_CHANNEL`) to match your network setup.

4. **Upload the Code**:
   - Open the code in Arduino IDE and upload it to your ESP32 board.

5. **Power the Device**:
   - Once uploaded, power the ESP32 and ensure all connections are secure.

---

## Code Explanation

### Main Components

1. **Fall Detection**:
   - Monitors acceleration data (`aX`, `aY`, `aZ`) from the MPU6050.
   - Detects significant fluctuations and verifies stability within a 2-second window.

2. **ESP-NOW Communication**:
   - Broadcasts messages to notify of detected falls or safe signals.

3. **Button Handling**:
   - Implements debounce logic to reliably detect button presses.
   - Sends a "safe signal" when the button is pressed, resetting the fall state.

### Tasks

- **`sendDataTask`**:
  - Continuously reads data from the MPU6050.
  - Processes fall detection and sends alerts via ESP-NOW.

- **`readButtonTask`**:
  - Monitors the state of the button.
  - Sends a "safe signal" when the button is pressed.

---

## Usage

1. The system continuously monitors accelerometer data to detect falls.
2. If a fall is detected:
   - An alert message is sent via ESP-NOW.
   - The system waits for a "safe signal" (button press) for 10 seconds.
3. Press the button to reset the fall status and send a "safe signal".

---

## Notes

- Adjust the fall threshold (`fallThreshold`) and debounce delay (`DEBOUNCE_DELAY`) in the code to match your use case.
- Ensure devices using ESP-NOW are on the same Wi-Fi channel.

---

## Troubleshooting

- **No Data from MPU6050**:
  - Check I2C connections and ensure proper wiring.
  - Verify the MPU6050 is powered.

- **No Alerts**:
  - Ensure the ESP-NOW peer is initialized successfully.
  - Verify the ESP32 is broadcasting messages.

---

## Future Improvements

- Add a web interface for monitoring fall status.
- Integrate with Blynk or similar IoT platforms for remote alerts.
- Enhance fall detection algorithms for better accuracy.

---

## License

This project is licensed under the SteadyStep License. Feel free to use and modify it.
