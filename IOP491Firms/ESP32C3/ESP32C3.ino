#include "ESP32_NOW.h"
#include "WiFi.h"
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <esp_mac.h>  // For the MAC2STR and MACSTR macros

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 6
#define BUTTON_PIN 2       // Chân kết nối nút (có thể thay đổi tùy theo thiết kế)
#define DEBOUNCE_DELAY 50  // Độ trễ chống dội nút (milisecond)
// Define global variables for storing incoming data.
float prev_aX = 0, prev_aY = 0, prev_aZ = 0;

// Define global variables for checking fall conditions.
bool isFall = false;              // Variable to store the fall status (true if fall detected)
unsigned long fallCheckTime = 0;  // Stores the time when we start checking after detecting a large movement
bool checkForMovement = false;    // Flag to indicate if we're currently in the 3-second check period
unsigned long last_time = 0;
unsigned long fallStartTime = 0;  // Thời gian bắt đầu phát hiện té ngã


unsigned long lastDebounceTime = 0;  // Lưu thời gian lần cuối nút thay đổi trạng thái
bool buttonState = LOW;              // Trạng thái hiện tại của nút
bool lastButtonState = LOW;          // Trạng thái trước đó của nút
bool buttonPressed = false;          // Biến đánh dấu khi nút được nhấn

bool checkingPatientStatusButton = false;
// define global variables for new algorithms
unsigned long lastTime = 0;
float prevAccX = 0.0, prevAccY = 0.0, prevAccZ = 0.0;
const float fallThreshold = 650000;  // Adjust this value to suit your needs (in m/s^3)
void checkForFluctuations2(float aX, float aY, float aZ);


bool waitingForSafeSignal = false;  // Đang chờ tín hiệu "PATIENT IS SAFE"
unsigned long waitStartTime = 0;    // Thời gian bắt đầu chờ
bool isStartCheckingSafeSignal = false;

/* Classes */
class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {
  }
  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("Failed to initialize ESP-NOW or register the broadcast peer");
      return false;
    }
    return true;
  }

  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      log_e("Failed to broadcast message");
      return false;
    }
    return true;
  }
};

/* Global Variables */
ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
Adafruit_MPU6050 mpu;

/* Tasks */
void sendDataTask(void *pvParameters);
void readButtonTask(void *pvParameters);

void setup() {
  // Serial.begin(115200);

  // Initialize button pin
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Initialize Wi-Fi and ESP-NOW
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  if (!broadcast_peer.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    delay(5000);
    ESP.restart();
  }

  // Initialize MPU6050
  if (!mpu.begin()) {
    // Serial.println("Failed to find MPU6050 chip");
    while (1) { delay(10); }
  }
  mpu.setHighPassFilter(MPU6050_HIGHPASS_0_63_HZ);
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_184_HZ);

  // Create tasks
  xTaskCreate(sendDataTask, "Send Data Task", 3000, NULL, 1, NULL);
  xTaskCreate(readButtonTask, "Read Button Task", 3000, NULL, 1, NULL);
}

void loop() {
  // The loop is intentionally left empty as tasks handle the workload.
}

// void sendDataTask(void *pvParameters) {
//   while (true) {
//     sensors_event_t a, g, temp;
//     // Read MPU data
//     mpu.getEvent(&a, &g, &temp);

//     // Prepare data for sending
//     char data[64];
//     snprintf(data, sizeof(data), "%.2f, %.2f, %.2f", a.acceleration.x, a.acceleration.y, a.acceleration.z);
//     checkForFluctuations2(a.acceleration.x, a.acceleration.y, a.acceleration.z);
//     // Send data over ESP-NOW
//     if (!broadcast_peer.send_message((uint8_t *)data, sizeof(data))) {
//       // Serial.println("Failed to send data");
//     }

//     if (isFall) {
//       // fallStartTime = millis();
//       // Serial.println("START COUNTING FALL TIME TO RESET");
//       // Create and send the fall detection signal
//       char fallSignal[] = "FallSignal:(FALL_DETECTED)";
//       if (!broadcast_peer.send_message((uint8_t *)fallSignal, strlen(fallSignal))) {
//         // Serial.println("Failed to broadcast fall signal");
//       }
//       Serial.println("FALL_DETECTED");
//       vTaskDelay(10000 / portTICK_PERIOD_MS);
//       isFall = false;  // Reset trạng thái té ngã
//     }
//     // Delay to match the sending frequency
//     vTaskDelay(10 / portTICK_PERIOD_MS);
//   }
// }


void sendDataTask(void *pvParameters) {
  while (true) {
    sensors_event_t a, g, temp;
    // Đọc dữ liệu từ MPU6050
    mpu.getEvent(&a, &g, &temp);
    // Kiểm tra các biến động để phát hiện té ngã
    checkForFluctuations2(a.acceleration.x, a.acceleration.y, a.acceleration.z);
    if (isFall && waitingForSafeSignal) {
      // Khi phát hiện té ngã, bắt đầu chờ tín hiệu "PATIENT IS SAFE"
      Serial.println("Fall detected, starting 10s wait for PATIENT IS SAFE...");
      waitingForSafeSignal = false;
      isStartCheckingSafeSignal = true;
      waitStartTime = millis();
    }
    if (!waitingForSafeSignal && isStartCheckingSafeSignal == true) {
      // Kiểm tra nếu nhận được tín hiệu "PATIENT IS SAFE"
      if (checkingPatientStatusButton) {
        Serial.println("PATIENT IS SAFE signal received.");
        waitingForSafeSignal = false;  // Dừng chờ
        isFall = false;                // Reset trạng thái té ngã
        isStartCheckingSafeSignal = false;
      }
      // Nếu vượt quá 10 giây mà không nhận được tín hiệu
      if (millis() - waitStartTime >= 10000) {
        Serial.println("10s elapsed, sending FALL_DETECTED signal...");
        char fallSignal[] = "FallSignal:(FALL_DETECTED)";
        if (!broadcast_peer.send_message((uint8_t *)fallSignal, strlen(fallSignal))) {
          // Serial.println("Failed to broadcast FALL_DETECTED signal");
        }
        // vTaskDelay(10000 / portTICK_PERIOD_MS);
        isStartCheckingSafeSignal = false;
        waitingForSafeSignal = false;  // Dừng chờ
        isFall = false;                // Reset trạng thái té ngã
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void readButtonTask(void *pvParameters) {
  while (true) {
    bool reading = digitalRead(BUTTON_PIN);
    if (reading != lastButtonState) {
      lastDebounceTime = millis();  
    }
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
      // Nếu trạng thái nút thay đổi
      if (reading != buttonState) {
        buttonState = reading;

        // Kiểm tra trạng thái nút
        if (buttonState == LOW) {
          // Nút được nhấn
          buttonPressed = true;
        } else if (buttonState == HIGH && buttonPressed) {
          // Nút được nhả và trước đó đã được nhấn
          // Serial.println("Patient is safe");
          checkingPatientStatusButton = true;
          buttonPressed = false;  // Reset trạng thái
        }
      }
    }

    // Lưu trạng thái lần đọc cuối
    lastButtonState = reading;
    if (checkingPatientStatusButton) {
      Serial.println("PATIENT IS SAFE");
      waitingForSafeSignal = false;
      isFall = false;
      isStartCheckingSafeSignal = false;
      // Nút nhấn không được ấn, gửi tín hiệu té ngã
      char fallSignal[] = "SafeSignal:(PATIENT_IS_SAFE)";
      if (!broadcast_peer.send_message((uint8_t *)fallSignal, strlen(fallSignal))) {
        // Serial.println("Failed to broadcast fall signal");
      }
      checkingPatientStatusButton = false;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

// void readButtonTask(void *pvParameters) {
//   while (true) {
//     bool reading = digitalRead(BUTTON_PIN);

//     if (reading != lastButtonState) {
//       lastDebounceTime = millis();
//     }

//     if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
//       if (reading != buttonState) {
//         buttonState = reading;

//         if (buttonState == LOW) {
//           buttonPressed = true;
//         }
//       }
//     }

//     lastButtonState = reading;

//     // Gửi tín hiệu SAFE nếu nút được nhấn
//     if (buttonPressed) {
//       Serial.println("PATIENT IS SAFE");
//       isFall = false;
//       char safeSignal[] = "SafeSignal:(PATIENT_IS_SAFE)";
//       if (!broadcast_peer.send_message((uint8_t *)safeSignal, strlen(safeSignal))) {
//         Serial.println("Failed to broadcast PATIENT IS SAFE signal");
//       }

//       // Reset trạng thái sau khi gửi
//       buttonPressed = false;
//     }

//     vTaskDelay(50 / portTICK_PERIOD_MS);
//   }
// }


// Function to check for fluctuations2
void checkForFluctuations2(float aX, float aY, float aZ) {
  static unsigned long detectionStartTime = 0;  // Lưu thời gian bắt đầu phát hiện biến động ±45
  static bool waitingForResetCheck = false;     // Cờ để theo dõi trạng thái chờ 2 giây

  // Phát hiện biến động lớn vượt ngưỡng ±45
  if (abs(aX - prev_aX) >= 45.0 || abs(aY - prev_aY) >= 45.0 || abs(aZ - prev_aZ) >= 45.0) {
    detectionStartTime = millis();  // Ghi nhận thời gian phát hiện biến động lớn
    waitingForResetCheck = true;    // Bật cờ chờ kiểm tra lại
    // Optional: Debugging output
    // Serial.println("Significant fluctuation detected. Waiting for 2 seconds...");
  }

  // Nếu đang chờ 2 giây để kiểm tra reset
  if (waitingForResetCheck) {
    // Kiểm tra nếu đã hết 2 giây
    if (millis() - detectionStartTime >= 2000) {
      // Kiểm tra biến động nhỏ vượt ±1 trong thời gian này
      if (abs(aX - prev_aX) > 1.0 || abs(aY - prev_aY) > 1.0 || abs(aZ - prev_aZ) > 1.0) {
        // Reset toàn bộ giá trị
        prev_aX = 0;
        prev_aY = 0;
        prev_aZ = 0;
        detectionStartTime = millis();  // Đặt lại thời gian
        waitingForResetCheck = false;   // Tắt cờ chờ
        isFall = false;                 // Reset trạng thái té ngã
        checkForMovement = false;       // Dừng kiểm tra chuyển động
        // Optional: Debugging output
        // Serial.println("RESET: Minor fluctuation exceeded ±1. Restarting check.");
        return;  // Thoát để bắt đầu lại
      } else {
        // Không có biến động lớn sau 2 giây, tiếp tục quy trình kiểm tra té ngã
        waitingForResetCheck = false;  // Tắt cờ chờ
        checkForMovement = true;       // Tiếp tục kiểm tra
        fallCheckTime = millis();      // Ghi nhận thời gian bắt đầu kiểm tra tiếp theo
        // Optional: Debugging output
        // Serial.println("No minor fluctuation. Continuing to fall detection...");
      }
    }
  }

  // Nếu đang trong quá trình kiểm tra (sau khi phát hiện ±45 và không bị reset)
  if (checkForMovement) {
    // Nếu hết thời gian kiểm tra (1 phút) hoặc có điều kiện phát hiện té ngã
    if (millis() - fallCheckTime >= 5000) {
      // Kiểm tra nếu không có biến động vượt ±1
      if (abs(aX - prev_aX) <= 1.0 && abs(aY - prev_aY) <= 1.0 && abs(aZ - prev_aZ) <= 1.0) {
        isFall = true;  // Xác nhận té ngã
        // Serial.println("FALL_DETECTED");
        waitingForSafeSignal = true;
      }
      // Kết thúc kiểm tra
      checkForMovement = false;
      prev_aX = 0;
      prev_aY = 0;
      prev_aZ = 0;
    }
  }

  // Cập nhật giá trị trước cho lần kiểm tra tiếp theo
  prev_aX = aX;
  prev_aY = aY;
  prev_aZ = aZ;
}
