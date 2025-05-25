/*
Final Detection Subsystem code.
Done By: Matthew Rathbone.

References which assisted in constructin this code: 
- https://github.com/techiesms/ESP32Cam_GPT4o.git
- https://github.com/gsampallo/esp32cam-gdrive.git
*/ 


#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include <Base64.h>
#include <base64v2.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "sensor.h"

// Wi-Fi Credentials
const char* wifiName = "";
const char* wifiPass = "";

// OpenAI API
const String prompt = "Honeybadger, leopard, caracal or other? 2 words MAX";
const String deploymentID = "";

#define LED_INDICATOR 4
#define MOTION_SENSOR 13

#define CAMERA_WAIT 2000
#define TIMEOUT_LIMIT 30000

// ESP32-CAM pin mapping
#define CAM_PWDN 32
#define CAM_RESET -1
#define CAM_XCLK 0
#define CAM_SIOD 26
#define CAM_SIOC 27
#define CAM_Y9 35
#define CAM_Y8 34
#define CAM_Y7 39
#define CAM_Y6 36
#define CAM_Y5 21
#define CAM_Y4 19
#define CAM_Y3 18
#define CAM_Y2 5
#define CAM_VSYNC 25
#define CAM_HREF 23
#define CAM_PCLK 22

String convertToBase64(const uint8_t* data, size_t length) {
  return base64::encode(data, length);
}

void setup()
  connectWiFi();
  delay(2000);
  configureCamera();
  delay(2000);
  analyzeScene();
}

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifiName, wifiPass);
  while (WiFi.status() != WL_CONNECTED) delay(500);
}

void configureCamera() {
  camera_config_t camSettings = {
    .pin_pwdn = CAM_PWDN,
    .pin_reset = CAM_RESET,
    .pin_xclk = CAM_XCLK,
    .pin_sscb_sda = CAM_SIOD,
    .pin_sscb_scl = CAM_SIOC,
    .pin_d7 = CAM_Y9,
    .pin_d6 = CAM_Y8,
    .pin_d5 = CAM_Y7,
    .pin_d4 = CAM_Y6,
    .pin_d3 = CAM_Y5,
    .pin_d2 = CAM_Y4,
    .pin_d1 = CAM_Y3,
    .pin_d0 = CAM_Y2,
    .pin_vsync = CAM_VSYNC,
    .pin_href = CAM_HREF,
    .pin_pclk = CAM_PCLK,
    .xclk_freq_hz = 20000000,
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_VGA,
    .jpeg_quality = 15,
    .fb_count = 1
  };

  if (esp_camera_init(&camSettings) != ESP_OK) {
    InterfaceSubsystem.println("Camera Initialization Failed");
    blinkLED(3);
    esp_deep_sleep_start();
  }
}

void analyzeScene() {
  camera_fb_t* pic = esp_camera_fb_get();
  if (!pic) {
    InterfaceSubsystem.println("Failed to capture image");
    return;
  }

  String image64 = convertToBase64(pic->buf, pic->len);
  esp_camera_fb_return(pic);

  if (image64.isEmpty()) {
    InterfaceSubsystem.println("Image Encoding Failed");
    return;
  }

  interpretImage(image64);
}

void interpretImage(const String& image64) {
  DynamicJsonDocument jsonDoc(4096);
  jsonDoc["model"] = "gpt-4o";

  auto messages = jsonDoc.createNestedArray("messages");
  auto message = messages.createNestedObject();
  message["role"] = "user";
  
  auto content = message.createNestedArray("content");
  content.createNestedObject().set("type", "text").set("text", prompt);

  auto imageBlock = content.createNestedObject();
  imageBlock["type"] = "image_url";
  imageBlock["image_url"]["url"] = "data:image/jpeg;base64," + image64;
  imageBlock["image_url"]["detail"] = "auto";

  jsonDoc["max_tokens"] = 5;

  String payload;
  serializeJson(jsonDoc, payload);

  String response;
  if (postToOpenAI(payload, response)) {
    DynamicJsonDocument responseJson(4096);
    deserializeJson(responseJson, response);
    String label = responseJson["choices"][0]["message"]["content"].as<String>();
    label.toLowerCase();

    if (label.indexOf("honey") >= 0) InterfaceSubsystem.println("Honey");
    else if (label.indexOf("leopard") >= 0) InterfaceSubsystem.println("Leopard");
    else if (label.indexOf("caracal") >= 0) InterfaceSubsystem.println("Caracal");
    else InterfaceSubsystem.println(label);
  } else {
    InterfaceSubsystem.println("OpenAI Error: " + response);
    esp_deep_sleep_start();
  }
}

bool postToOpenAI(const String& jsonData, String& output) {
  HTTPClient http;
  http.begin("https://api.openai.com/v1/chat/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + deploymentID);
  http.setTimeout(20000);

  int code = http.POST(jsonData);
  if (code > 0) {
    output = http.getString();
    http.end();
    return true;
  } else {
    output = "HTTP Error: " + String(code);
    InterfaceSubsystem.println(output);
    blinkLED(2);
    http.end();
    return false;
  }
}

String urlEncode(String s) {
  String encoded = "";
  char c;
  char hex[4];
  for (unsigned int i = 0; i < s.length(); i++) {
    c = s.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else if (c == ' ') {
      encoded += '+';
    } else {
      sprintf(hex, "%%%02X", c);
      encoded += hex;
    }
    yield();
  }
  return encoded;
}

void blinkLED(int times) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_INDICATOR, HIGH);
    delay(500);
    digitalWrite(LED_INDICATOR, LOW);
    delay(500);
  }
}

void loop() {
  esp_sleep_enable_ext0_wakeup((gpio_num_t)MOTION_SENSOR, 0);
  esp_deep_sleep_start();
}
