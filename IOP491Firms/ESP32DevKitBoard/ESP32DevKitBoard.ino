#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>  // For the MAC2STR and MACSTR macros
#include <vector>
#include <string.h>

/* Definitions */
#define ESPNOW_WIFI_CHANNEL 6
#define BUZZER_PIN 18  // Chân điều khiển Buzzer

/* Global Variables */
String extractedFallSignalContent = "";
String extractedSafeSignalContent = "";
bool isFallDetected = false;      // Trạng thái Fall Detected
unsigned long fallStartTime = 0;  // Thời gian bắt đầu cảnh báo

/* Function Declarations */
String extractFallSignalContent(const char *input);
String extractSafeSignalContent(const char *input);
void risefall();
void triggerBuzzer();

/* Classes */
class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
    : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}
  ~ESP_NOW_Peer_Class() {}

  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    String receivedData = String((char *)data);
    String isFallSignal = "FALL_DETECTED";
    String isSafeSignal = "PATIENT_IS_SAFE";
    String fallContent = extractFallSignalContent(receivedData.c_str());
    String safeContent = extractSafeSignalContent(receivedData.c_str());
    // Serial.printf("Received raw message: %s\n", (char *)data);
    if (!fallContent.isEmpty()) {
      extractedFallSignalContent = fallContent;  // Lưu vào biến toàn cục
      if (extractedFallSignalContent == isFallSignal) {
        isFallDetected = true;  // Bật trạng thái cảnh báo
        Serial.print("FALL_DETECTED");
        triggerBuzzer();
        fallStartTime = millis();  // Lưu thời gian bắt đầu
      }
      // Serial.printf("Extracted fallContent: %s\n", extractedFallSignalContent.c_str());
    } else if (!safeContent.isEmpty() && safeContent == isSafeSignal) {
      isFallDetected = false;
      noTone(BUZZER_PIN);
    }
  }
  // void onReceive(const uint8_t *data, size_t len, bool broadcast) {
  //   String receivedData = String((char *)data);
  //   String isFallSignal = "FALL_DETECTED";
  //   String isSafeSignal = "PATIENT_IS_SAFE";
  //   Serial.println(receivedData);
  //   String fallContent = extractFallSignalContent(receivedData.c_str());
  //   String safeContent = extractSafeSignalContent(receivedData.c_str());
  //   // Kiểm tra tín hiệu nhận được
  //   if (fallContent == isFallSignal ) {
  //     // Nhận tín hiệu FALL_DETECTED
  //     Serial.println(fallContent);
  //     isFallDetected = true;  // Bật trạng thái cảnh báo
  //     // fallStartTime = millis();  // Lưu thời gian bắt đầu
  //     Serial.println("FALL_DETECTED signal received. Waiting for SAFE signal...");
  //     // Đợi 5 giây để kiểm tra tín hiệu SAFE
  //     unsigned long waitStartTime = millis();
  //     bool safeSignalReceived = false;
  //     while (millis() - waitStartTime < 5000) {
  //       // Kiểm tra nếu nhận được tín hiệu SAFE
  //       if (safeContent == isSafeSignal) {
  //         Serial.println(safeContent);
  //         safeSignalReceived = true;
  //         break;
  //       }
  //       delay(10);  // Giảm tải CPU trong vòng lặp
  //     }
  //     if (!safeSignalReceived) {
  //       // Nếu không nhận được SAFE signal trong 5 giây
  //       Serial.println("FALL_DETECTED");
  //       triggerBuzzer();           // Kích hoạt còi
  //       fallStartTime = millis();  // Lưu thời gian bắt đầu
  //     } else {
  //       // Nếu nhận được tín hiệu SAFE
  //       Serial.println("SAFE signal received. No action needed.");
  //     }
  //   } else if (safeContent == isSafeSignal) {
  //     // Nhận tín hiệu SAFE trực tiếp
  //     isFallDetected = false;
  //     noTone(BUZZER_PIN);
  //     Serial.println("PATIENT_IS_SAFE signal received.");
  //   }
  // }
};

/* Global Variables */
std::vector<ESP_NOW_Peer_Class> masters;

/* Callbacks */
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    // Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    // Serial.println("Registering the peer as a master");

    ESP_NOW_Peer_Class new_master(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, NULL);
    masters.push_back(new_master);
    if (!masters.back().add_peer()) {
      // Serial.println("Failed to register the new master");
      return;
    }
  }
}

/* Main */
void setup() {
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) {
    delay(100);
  }

  // Cấu hình chân GPIO
  pinMode(BUZZER_PIN, OUTPUT);

  // Tắt Buzzer ban đầu
  digitalWrite(BUZZER_PIN, LOW);

  // Serial.println("ESP-NOW Slave - Receiving MPU Data");
  // Serial.println("Wi-Fi parameters:");
  // Serial.println("  MAC Address: " + WiFi.macAddress());
  // Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW
  if (!ESP_NOW.begin()) {
    // Serial.println("Failed to initialize ESP-NOW");
    delay(5000);
    ESP.restart();
  }

  // Register the new peer callback
  ESP_NOW.onNewPeer(register_new_master, NULL);

  // Serial.println("Setup complete. Waiting for a master to broadcast a message...");
}

void loop() {
  // Kiểm tra nếu Buzzer đang hoạt động và đã qua 3 giây
  if (isFallDetected && millis() - fallStartTime >= 30000) {
    // Serial.println("Turning off buzzer after 3 seconds.");
    isFallDetected = false;
    noTone(BUZZER_PIN);
  }
  delay(100);
}

/* Function Definitions */
String extractFallSignalContent(const char *input) {
  const char *start = strstr(input, "FallSignal:(");
  if (start) {
    start += 12;  // Bỏ qua "Signal:("
    const char *end = strchr(start, ')');
    if (end) {
      size_t len = end - start;
      char buffer[len + 1];
      strncpy(buffer, start, len);
      buffer[len] = '\0';  // Đảm bảo kết thúc chuỗi
      return String(buffer);
    }
  }
  return String("");  // Trả về chuỗi rỗng nếu không tìm thấy định dạng
}

String extractSafeSignalContent(const char *input) {
  const char *start = strstr(input, "SafeSignal:(");
  if (start) {
    start += 12;  // Bỏ qua "Signal:("
    const char *end = strchr(start, ')');
    if (end) {
      size_t len = end - start;
      char buffer[len + 1];
      strncpy(buffer, start, len);
      buffer[len] = '\0';  // Đảm bảo kết thúc chuỗi
      return String(buffer);
    }
  }
  return String("");  // Trả về chuỗi rỗng nếu không tìm thấy định dạng
}




void triggerBuzzer() {
  for (int count = 1; count <= 20; count++) {
    risefall();
  }
}

void risefall() {
  float rise_fall_time = 180;
  int steps = 50;
  float f_max = 2600;
  float f_min = 1000;
  float delay_time = rise_fall_time / steps;
  float step_size = (f_max - f_min) / steps;
  for (float f = f_min; f < f_max; f += step_size) {
    tone(BUZZER_PIN, f);
    delay(delay_time);
  }
  for (float f = f_max; f > f_min; f -= step_size) {
    tone(BUZZER_PIN, f);
    delay(delay_time);
  }
}
