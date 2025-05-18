#include <WiFiClientSecure.h>         // Secure connection library (for HTTPS)
#include "soc/soc.h"                  // Low-level system configuration
#include "soc/rtc_cntl_reg.h"         // Allows for alterations to brown out detect
#include "sensor.h"                   // Camera configuration
#include <ArduinoJson.h>              // JSON request header
#include <WiFi.h>                     // Standard WiFi stuff
#include <Base64.h>                   // Uses Esspresif ESP32 Libraru
#include <base64v2.h>                 // Custom Base64 for base64 Encoding
#include <HTTPClient.h>               // HTTP requests header file
#include "esp_camera.h"               // Camera Driver

// WiFi credentials
const char* ssid     = "NETWORK NAME HERE";   
const char* password = "PASSWORD HERE";   

// Google Script Information
const char* myDomain = "script.google.com";
String myScript = "Deployment ID Here";
String myFilename = "filename=ESP32-CAM.jpg";
String mimeType = "&mimetype=image/jpeg";
String myImage = "&data=";

// Question to ask ChatGPT about the image
String Question = "Honeybadger, leopard, caracal or other? 2 words MAX";

// Your OpenAI API key
const String apiKey = ""; 

int waitingTime = 30000; // Max time to wait for server response in ms

// ESP32-CAM pinout configuration
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define PIR_PIN 13            // PIR sensor input
#define GPIO_ERROR 33         // Error display uses onload LED
#define LED_PIN 4             // Onboard LED
#define GPIO_PIN_1 1          // Serial TX
#define GPIO_PIN_3 3          // Serial RX

// Serial communication with Interface subsystem
HardwareSerial interfaceSystem(0);

// Just a helper function to base64 encode the image buffer using custom base64 imported file.
String encodeBase64(const uint8_t* imageData, size_t imageSize) 
{
  return base64::encode(imageData, imageSize);
}

void setup() 
{
  // Disable the brownout detector
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  // Initialize serial communication with custom RX/TX pins
  interfaceSystem.begin(115200, SERIAL_8N1, GPIO_PIN_3, GPIO_PIN_1);

  // Configure LED and PIR sensor pins
  pinMode(LED_PIN, OUTPUT);
  pinMode(GPIO_NUM_13, INPUT_PULLUP); // PIR sensor (active low)

  // Blink LED as startup indicator
  digitalWrite(LED_PIN, true);
  delay(1000);
  digitalWrite(LED_PIN, false);
  delay(1000);
  digitalWrite(LED_PIN, LOW);

  // Determine wake-up reason
  esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

  // Enable external wake-up via PIR sensor
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 0);
  delay(2000);
  
  // If woken by PIR sensor and it remains triggered
  if (wakeup_reason == ESP_SLEEP_WAKEUP_EXT0 && digitalRead(GPIO_NUM_13)==false)
  {
    interfaceSystem.println(String("Light\n"));  // Notify interface subsystem
    wifiConfig();           // Configure Wi-Fi
    delay(2000);
    cameraConfig();         // Initialize camera
    delay(2000);
    chatGPTAnalysis();      // Capture image and send to OpenAI
    googleDriveSave();      // Upload image to Google Apps Script
    esp_deep_sleep_start(); // Return to deep sleep
  } 

  // Return to deep sleep if not woken by PIR or once it 
  esp_deep_sleep_start();
}

// Establishes Wi-Fi connection
void wifiConfig()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);  

  // Wait until Wi-Fi is connected
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

// Configures the camera settings
void cameraConfig()
{
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  
  config.jpeg_quality = 15;  // Lower value means better quality
  config.fb_count = 1;

  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    // If initialization fails, signal error with LED and sleep
    digitalWrite(LED_PIN, true);
    delay(4000);
    digitalWrite(LED_PIN, false);
    delay(1000);
    esp_deep_sleep_start();
  }

  delay(2000); // Delay after camera initialization
}

// Sends an HTTP POST request to OpenAI
bool sendPostRequest(const String& payload, String& result) 
{
  HTTPClient http;
  http.begin("https://api.openai.com/v1/chat/completions");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + apiKey);
  http.setTimeout(20000); // Set request timeout to 20 seconds

  int httpResponseCode = http.POST(payload);
  if (httpResponseCode > 0) {
    result = http.getString();
    http.end();
    return true;
  } else {
    result = "HTTP request failed, response code: " + String(httpResponseCode);
    digitalWrite(LED_PIN, true);
    delay(4000);
    digitalWrite(LED_PIN, false);
    http.end();
    return false;
  }
}

void loop() 
{
  // Re-enable wake-up and enter deep sleep until triggered
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIR_PIN, 0);
  esp_deep_sleep_start();
}

// Captures and uploads an image to Google Apps Script
void googleDriveSave() 
{
  WiFiClientSecure client;
  client.setInsecure(); // Disable SSL certificate validation

  if (client.connect(myDomain, 443)) {

    // Flip the image vertically and horizontally if required
    sensor_t * s = esp_camera_sensor_get();
    if (true)
    {
      s->set_hmirror(s, 1);
      s->set_vflip(s, 1);
    }

    // Capture image
    camera_fb_t * fb = NULL;
    fb = esp_camera_fb_get();  
    if(!fb) {
      interfaceSystem.println(String("Camera capture failed.\n"));
      delay(1000);
      ESP.restart();
      return;
    }

    // Manually base64 encode image for compatibility
    char *input = (char *)fb->buf;
    char output[base64_enc_len(3)];
    String imageFile = "";
    for (int i=0;i<fb->len;i++) {
      base64_encode(output, (input++), 3);
      if (i%3==0) imageFile += urlencode(String(output));
    }

    String Data = myFilename + mimeType + myImage;
    esp_camera_fb_return(fb);  // Release frame buffer

    // Send HTTP POST request to Google Apps Script
    client.println("POST " + myScript + " HTTP/1.1");
    client.println("Host: " + String(myDomain));
    client.println("Content-Length: " + String(Data.length() + imageFile.length()));
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println();
    client.print(Data);

    // Send the image data in chunks
    int Index;
    for (Index = 0; Index < imageFile.length(); Index = Index + 1000) {
      client.print(imageFile.substring(Index, Index + 1000));
    }

    // Wait for response or timeout
    long int StartTime = millis();
    while (!client.available()) {
      delay(100);
      if ((StartTime + waitingTime) < millis()) {
        digitalWrite(LED_PIN, true);
        delay(4000);
        digitalWrite(LED_PIN, false);
        break;
      }
    } 
  }
  client.stop();
  esp_deep_sleep_start(); // Return to deep sleep
}

// Encodes a string for safe URL transmission
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i = 0; i < str.length(); i++) {
      c = str.charAt(i);
      if (c == ' '){
        encodedString += '+';
      } else if (isalnum(c)) {
        encodedString += c;
      } else {
        code1 = (c & 0xf) + '0';
        if ((c & 0xf) > 9){
          code1 = (c & 0xf) - 10 + 'A';
        }
        c = (c >> 4) & 0xf;
        code0 = c + '0';
        if (c > 9){
          code0 = c - 10 + 'A';
        }
        code2 = '\0';
        encodedString += '%';
        encodedString += code0;
        encodedString += code1;
      }
      yield();
    }
    return encodedString;
}

// Captures image, encodes it, and sends to OpenAI for analysis
void  chatGPTAnalysis() 
{
  camera_fb_t* fb = esp_camera_fb_get();  // Capture image
  if (!fb) {
    interfaceSystem.println(String("Camera capture failed\n"));
    return;
  }

  esp_camera_fb_return(fb);  // Release memory
  fb = esp_camera_fb_get();  // Capture a fresh image

  if (!fb) return;

  String base64Image = encodeBase64(fb->buf, fb->len); // Base64 encode image

  esp_camera_fb_return(fb); // Release memory

  if (base64Image.isEmpty()) {
    interfaceSystem.println(String("Failed to encode the image!\n"));
    return;
  }

  AnalyzeImage(base64Image); // Analyze image using OpenAI
}

// Sends a base64 image to OpenAI and processes the response
void AnalyzeImage(const String& base64Image) 
{
  String result;
  String url = "data:image/jpeg;base64," + base64Image;

  DynamicJsonDocument doc(4096);
  doc["model"] = "gpt-4o";
  JsonArray messages = doc.createNestedArray("messages");
  JsonObject message = messages.createNestedObject();
  message["role"] = "user";
  JsonArray content = message.createNestedArray("content");

  JsonObject textContent = content.createNestedObject();
  textContent["type"] = "text";
  textContent["text"] = Question;

  JsonObject imageContent = content.createNestedObject();
  imageContent["type"] = "image_url";
  JsonObject imageUrlObject = imageContent.createNestedObject("image_url");
  imageUrlObject["url"] = url;
  imageContent["image_url"]["detail"] = "auto";

  doc["max_tokens"] = 5;

  String jsonPayload;
  serializeJson(doc, jsonPayload);

  // Send request and parse OpenAI response
  if (sendPostRequest(jsonPayload, result)) {
    DynamicJsonDocument responseDoc(4096);
    deserializeJson(responseDoc, result);

    String responseContent = responseDoc["choices"][0]["message"]["content"].as<String>();
    responseContent.toLowerCase();

    // Output a short code based on identified animal
    if(responseContent.indexOf("honey") != -1) {
      interfaceSystem.println(String("HB\n"));
    }
    else if(responseContent.indexOf("leopard") != -1) {
      interfaceSystem.println(String("LD\n"));
    }
    else if(responseContent.indexOf("caracal") != -1) {
      interfaceSystem.println(String("CL\n"));
    }
    else {
      interfaceSystem.println(responseContent);
    }

  } else {
    esp_deep_sleep_start(); // Return to sleep if analysis fails
  }
}