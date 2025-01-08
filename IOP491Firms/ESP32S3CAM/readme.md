# ESP32 Live Video Streaming with Firebase and Blynk

This project integrates an ESP32-S3 with a camera module to capture images, upload them to Firebase Storage, and update a Blynk widget with the uploaded image URL. It includes multi-tasking with FreeRTOS and various other capabilities like LittleFS and ESP-NOW.

## Features
- **Image Capture:** Captures images using an ESP32-S3 camera module.
- **Firebase Integration:** Uploads captured images to Firebase Storage.
- **Blynk Updates:** Updates Blynk with the URL of the uploaded image.
- **FreeRTOS Tasks:** Handles concurrent operations using separate tasks for image uploading, Blynk updates, and UART reception.
- **Persistent Storage:** Uses LittleFS for local file storage.

## Requirements
- ESP32-S3 board with a camera module.
- Firebase Storage and Authentication configured.
- Blynk account and template.
- Arduino IDE or PlatformIO for development.

## Wiring
- ESP32-S3 board with built-in or attached camera module.
- No external wiring is required for this example.

## Configuration

### Blynk Configuration
Update the Blynk parameters:
- `BLYNK_TEMPLATE_ID`: Template ID from Blynk.
- `BLYNK_TEMPLATE_NAME`: Template name from Blynk.
- `BLYNK_AUTH_TOKEN`: Authentication token.

### Firebase Configuration
Set up Firebase parameters:
- `API_KEY`: Firebase API key.
- `USER_EMAIL`: Firebase user email.
- `USER_PASSWORD`: Firebase user password.
- `STORAGE_BUCKET_ID`: Firebase storage bucket ID.

### Wi-Fi Configuration
Update your Wi-Fi credentials:
- `ssid`: Wi-Fi network SSID.
- `pass`: Wi-Fi network password.

## Code Overview

### Key Functions
- **`capturePhotoSaveLittleFS`**: Captures an image and saves it to LittleFS.
- **`uploadPhotoToFirebase`**: Uploads the saved image to Firebase and gets a download URL.
- **`updateBlynkPicture`**: Updates a Blynk widget with the image URL.
- **`initWiFi`, `initLittleFS`, and `initCamera`**: Initialize Wi-Fi, LittleFS, and the camera, respectively.
- **`BlynkRunTask`, `UARTReceivedTask`, and `ImageUploadTask`**: FreeRTOS tasks to handle Blynk operations, UART data reception, and image uploading.

### FreeRTOS Tasks
- **`BlynkRunTask`**: Keeps the Blynk connection alive and processes updates.
- **`UARTReceivedTask`**: Monitors incoming UART messages for specific triggers like `FALL_DETECTED`.
- **`ImageUploadTask`**: Captures and uploads images upon receiving triggers.