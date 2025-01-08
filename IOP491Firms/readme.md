# ESP32-S3 Fall Detection and Live Video Streaming Project

This repository contains code for an ESP32-S3-based project that integrates fall detection, live video streaming, Firebase Storage, and Blynk. It utilizes a camera module for capturing images, multi-tasking for efficient processing, and cloud services for remote access.

---

## Features
- **Fall Detection**: Monitors accelerometer data to detect falls and triggers image capture.
- **Live Video Streaming**: Captures and streams images to a Blynk widget.
- **Firebase Integration**: Uploads images to Firebase Storage with download URLs.
- **Multi-Tasking**: Uses FreeRTOS tasks for concurrent operations.
- **Local Storage**: Supports LittleFS and SD card storage for images.
- **UART Communication**: Listens for external commands via UART.

---

## Project Structure

This repository contains three main code files:

### 1. **Fall Detection Code**
Handles fall detection based on accelerometer fluctuations and UART commands to trigger image capture.

### 2. **Image Handling Code**
Manages camera operations, image storage, and updating the Blynk app with captured image URLs.

### 3. **Full System Integration**
Combines fall detection, image handling, Firebase uploads, and Blynk updates into one cohesive program.

---

## Requirements

### **Hardware**
- ESP32-S3 development board with a camera module.
- Accelerometer (e.g., MPU6050) for fall detection.
- SD card module (optional for external storage).

### **Software**
- Arduino IDE or PlatformIO.
- Firebase account for Storage and Authentication.
- Blynk account for real-time updates.

### **Libraries**
Install the following libraries in Arduino IDE:
- [Firebase ESP Client](https://github.com/mobizt/Firebase-ESP-Client)
- [Blynk Library](https://github.com/blynkkk/blynk-library)
- [ESP32 Camera Library](https://github.com/espressif/esp32-camera)
- LittleFS and SD libraries.
