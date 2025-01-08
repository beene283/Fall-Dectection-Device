/* Comment this out to disable prints and save space */
// #define BLYNK_PRINT Serial

// Blynk configuration
#define BLYNK_TEMPLATE_ID "TMPL6-3JBT6Sn"
#define BLYNK_TEMPLATE_NAME "Live Video Streaming Test Online1"
#define BLYNK_AUTH_TOKEN "elTdWigV9lEreKLYKyUpU_AFdkigbwf3"
// Pin definition for CAMERA_MODEL_ESP32S3_EYE
#define CAMERA_MODEL_ESP32S3_EYE  // Has PSRAM

#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include <LittleFS.h>
#include <FS.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <esp_now.h>
#include <WiFi.h>
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownout problems
#include "soc/rtc_cntl_reg.h"  // Disable brownout problems
#include <ESP32_FTPClient.h>
#include <BlynkSimpleEsp32.h>
#include <WiFiClient.h>
#include <EEPROM.h>  // read and write from flash memory
#include "camera_pins.h"


#define CHANNEL 1
#define FILE_PHOTO_PATH "/photo.jpg"
#define BUCKET_PHOTO "/data/photo.jpg"
#define API_KEY "AIzaSyBQK91aoZ_XXGuOBt_8y33eiFNtMhEx2SM"
#define USER_EMAIL "linhze2003@gmail.com"
#define USER_PASSWORD "linh123123"
#define STORAGE_BUCKET_ID "falldetectiop491.appspot.com"
char ssid[] = "LOUISVU";
char pass[] = "linhdeptraivcl";

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig configF;

// Define global variables for storing incoming data.
float prev_aX = 0, prev_aY = 0, prev_aZ = 0;

bool isFall = false;
unsigned long fallCheckTime = 0;
bool isChecking = false;
TaskHandle_t xHandleImageUpload = NULL;
TaskHandle_t xHandleBlynkRun = NULL;
TaskHandle_t xHandleUARTReceived = NULL;


bool takeNewPhoto;

void checkForFluctuations(float aX, float aY, float aZ);
void capturePhotoSaveLittleFS(void);
void uploadPhotoToFirebase(void);
void ImageUploadTask(void *pvParameters);
void fcsUploadCallback(FCS_UploadStatusInfo info);
void blynkInit();
void updateBlynkPicture(String imgURL);


void setup() {
  Serial.begin(115200);  // UART0, RX = GPIO3, TX = GPIO1
  initWiFi();
  initLittleFS();
  initCamera();
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
  configF.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  configF.token_status_callback = tokenStatusCallback;
  Firebase.begin(&configF, &auth);
  Firebase.reconnectWiFi(true);
  Serial.println("Freenove ESP32-S3 WROOM Board is ready to receive data.");
  xTaskCreatePinnedToCore(ImageUploadTask, "ImageUploadTask", 8000, NULL, 1, &xHandleImageUpload, 0);
  xTaskCreatePinnedToCore(UARTReceivedTask, "UARTReceivedTask", 5000, NULL, 3, &xHandleUARTReceived, 1);
  xTaskCreatePinnedToCore(BlynkRunTask, "RunningBlynkTask", 5000, NULL, 2, &xHandleBlynkRun, 0);
}

void BlynkRunTask(void *pvParameters) {
  while (true) {
    Blynk.run();
    vTaskDelay(1500 / portTICK_PERIOD_MS);
  }
}

void UARTReceivedTask(void *pvParameters) {
  while (true) {
    if (Serial.available() > 0) {
      String received = Serial.readStringUntil('\n');
      Serial.print("Received: ");
      Serial.println(received);
      if (received.indexOf("FALL_DETECTED") != -1) {
        takeNewPhoto = true;
      }
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);  // Yield to prevent WDT reset
  }
}

void ImageUploadTask(void *pvParameters) {
  bool testingTemp = true;
  while (true) {
    // if (Serial.available()) {
    //   String receivedData = Serial.readStringUntil('\n');
    //   Serial.println(receivedData);
    //   // Kiểm tra nếu receivedData chứa "FALL_DETECTED"
    //   if (receivedData.indexOf("FALL_DETECTED") != -1) {
    //     takeNewPhoto = true;
    //   }
    // }
    if (Firebase.ready() && takeNewPhoto == true) {
      capturePhotoSaveLittleFS();
      uploadPhotoToFirebase();
      isFall = false;  // Reset fall status after upload
      // vTaskSuspend(NULL);  // Suspend after uploading, resuming data reception
      takeNewPhoto = false;
    }
    vTaskDelay(500 / portTICK_PERIOD_MS);  // Yield to prevent WDT reset
  }
}

void capturePhotoSaveLittleFS(void) {
  Serial.println("TAKING PICTURE ..... ");
  camera_fb_t *fb = NULL;
  for (int i = 0; i < 4; i++) {
    fb = esp_camera_fb_get();
    esp_camera_fb_return(fb);
  }


  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    // ESP.restart();
  }

  File file = LittleFS.open(FILE_PHOTO_PATH, FILE_WRITE);
  file.write(fb->buf, fb->len);
  file.close();
  Serial.println("DONE TAKING PICTURE");
  esp_camera_fb_return(fb);
}

void uploadPhotoToFirebase(void) {
  if (Firebase.Storage.upload(&fbdo, STORAGE_BUCKET_ID, FILE_PHOTO_PATH, mem_storage_type_flash, BUCKET_PHOTO, "image/jpeg", fcsUploadCallback)) {
    Serial.printf("Download URL: %s\n", fbdo.downloadURL().c_str());
    delay(1000);
    updateBlynkPicture(fbdo.downloadURL());
  } else {
    Serial.println(fbdo.errorReason());
  }
}

void fcsUploadCallback(FCS_UploadStatusInfo info) {
  if (info.status == firebase_fcs_upload_status_complete) {
    Serial.println("Upload completed");
  }
}

void initWiFi() {
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}
void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
    ESP.restart();
  } else {
    Serial.println("LittleFS mounted successfully");
  }
}
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;        // Tần số xclk cho camera
  config.frame_size = FRAMESIZE_HD;      // Full HD (1600x1200)
  config.pixel_format = PIXFORMAT_JPEG;  // Định dạng ảnh JPEG (có thể nén với chất lượng tốt)
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;  // Dùng PSRAM để lưu trữ ảnh
  config.jpeg_quality = 10;                 // Chất lượng ảnh JPEG từ 0 (tốt nhất) đến 63 (kém nhất)
  config.fb_count = 2;                      // Sử dụng 2 frame buffer để giảm độ trễ

  // Kiểm tra PSRAM có sẵn không
  if (psramFound()) {
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;  // Lấy ảnh mới nhất từ buffer
  } else {
    config.frame_size = FRAMESIZE_SVGA;  // Giảm độ phân giải nếu không có PSRAM
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Khởi tạo camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();

  // Cấu hình lại sensor (cân chỉnh màu sắc và độ sáng) cho camera OV2640
  if (s->id.PID == OV2640_PID) {
    s->set_vflip(s, 1);                       // Lật ảnh dọc (nếu cần)
    s->set_brightness(s, 1);                  // Độ sáng (giá trị từ -2 đến 2)
    s->set_saturation(s, 0);                  // Mức độ bão hòa (giá trị từ -2 đến 2)
    s->set_contrast(s, 1);                    // Độ tương phản (giá trị từ -2 đến 2)
    s->set_special_effect(s, 0);              // Không sử dụng hiệu ứng đặc biệt
    s->set_whitebal(s, 1);                    // Kích hoạt cân bằng trắng
    s->set_awb_gain(s, 1);                    // Bật tự động cân bằng trắng
    s->set_wb_mode(s, 0);                     // Cân bằng trắng tự động
    s->set_exposure_ctrl(s, 1);               // Bật tự động điều chỉnh phơi sáng
    s->set_aec2(s, 0);                        // Tắt AEC2 (Auto Exposure Compensation)
    s->set_ae_level(s, 0);                    // Mức độ phơi sáng (giá trị từ -2 đến 2)
    s->set_aec_value(s, 300);                 // Điều chỉnh giá trị phơi sáng
    s->set_gain_ctrl(s, 1);                   // Bật điều khiển gain (tăng độ sáng)
    s->set_agc_gain(s, 0);                    // Điều chỉnh mức gain (từ 0 đến 30)
    s->set_gainceiling(s, (gainceiling_t)0);  // Giới hạn gain
    s->set_bpc(s, 0);                         // Tắt loại bỏ nhiễu ảnh (BPC)
    s->set_wpc(s, 1);                         // Bật giảm nhiễu (WPC)
    s->set_raw_gma(s, 1);                     // Kích hoạt Gamma
    s->set_lenc(s, 1);                        // Bật chỉnh sửa màu sắc
    s->set_hmirror(s, 0);                     // Tắt lật ảnh ngang
    s->set_vflip(s, 0);                       // Tắt lật ảnh dọc
    s->set_dcw(s, 1);                         // Bật Digital Color Width (DCW) để chỉnh sửa màu sắc
    s->set_colorbar(s, 0);                    // Tắt chế độ màu sắc
  }

  delay(1000);  // Đợi 1 giây để camera khởi tạo hoàn tất
}

void updateBlynkPicture(String imgURL) {
  // Update Blynk with image URL
  Blynk.setProperty(V1, "url", imgURL.c_str());
  Blynk.setProperty(V1, "offImageUrl", imgURL.c_str());
  Blynk.setProperty(V1, "onImageUrl", imgURL.c_str());
  Serial.println("Blynk.setProperty has been executed with image URL: " + imgURL);
}

void loop() {
}
