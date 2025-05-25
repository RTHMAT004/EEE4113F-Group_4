#include <HTTPClient.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <TimeLib.h>
#include <SPI.h>
#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// WiFi credentials
const char* ssid = "DESKTOP-QJB68QR 3134";
const char* password = "1{9H99s4";

// Google Apps Script URL
const char* scriptURL = "https://script.google.com/macros/s/AKfycby8O9UX5A4D2Daj9K-9-ITW31twISIjSR6UN3imDm-i9nSvniDLcfMIkt47pQGJMYuV/exec";

// Screen pins
#define D0 18
#define D1 23
#define RES 27
#define DC 26
#define CS 5

// UART pins (comms from detection)
#define UART_TX 17
#define UART_RX 16

// GPIO digital logic pins (comms to mitigation)
#define OUTPUT_PIN_1 12
#define OUTPUT_PIN_2 13
#define OUTPUT_PIN_3 14

// Screen object initialisation 
U8G2_SSD1306_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);

// Data
String captureTimestamps[3] = { "--", "--", "--" };
String unitNumber = "--";
String sBuffer = "";
SemaphoreHandle_t screenSemaphore;

void initialiseScreen() {
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tr);
  u8g2.drawStr(0, 12, "Connecting WiFi...");
  u8g2.sendBuffer();
}

void connectWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");

  // Confirm WiFi connection
  if (xSemaphoreTake(screenSemaphore, portMAX_DELAY)) {
    u8g2.clearBuffer();
    u8g2.drawStr(0, 12, "WiFi Connected!");
    u8g2.sendBuffer();
    xSemaphoreGive(screenSemaphore);
  }

  // Sync time from NTP (UTC+2)
  configTime(3600 * 2, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for time...");
    delay(1000);
  }
}

void writeScreen(const char* currentTime) {
  u8g2.setFont(u8g2_font_5x8_tr);
  u8g2.drawStr(0, 12, currentTime);
  u8g2.drawStr(0, 20, (String("Unit: ") + unitNumber).c_str());
  u8g2.drawFrame(0, 23, 100, 1);
  u8g2.drawFrame(100, 0, 1, 24);

  char line1[32], line2[32], line3[32];
  snprintf(line1, sizeof(line1), "1. %s", captureTimestamps[2].c_str());
  snprintf(line2, sizeof(line2), "2. %s", captureTimestamps[1].c_str());
  snprintf(line3, sizeof(line3), "3. %s", captureTimestamps[0].c_str());

  u8g2.drawStr(0, 35, line1);
  u8g2.drawStr(0, 45, line2);
  u8g2.drawStr(0, 55, line3);
}

void readFromGoogleSheet() {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(scriptURL);
  int code = http.GET();

  if (code == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error) {
      Serial.println("JSON parsing failed!");
      return;
    }

  JsonArray tsArray = doc["timestamps"];
  for (int i = 0; i < 3; i++) {
    if (i < tsArray.size()) {
      captureTimestamps[i] = tsArray[i].as<String>();
    } else {
      captureTimestamps[i] = "--";
    }
  }

  unitNumber = doc["unitNumber"] | "--";

  } else {
    Serial.println("HTTP error: " + String(code));
  }
  http.end();
}

void sendTrapTask(void* param) { 
while (true) {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c >= 32 && c <= 126) {
      sBuffer += c;
    }
  }

  if (sBuffer.length() > 20) {
    sBuffer = sBuffer.substring(sBuffer.length() - 20); //compose UART buffer of last 20 characters to check for incoming messages
  }

  Serial.println(String("Buffer: ") + sBuffer);

  int triggered = 0;

  if (sBuffer.indexOf("Honey") != -1) {             // OUTPUT 1 is siren and light
   // Serial.println("HONEY BADGER");
    digitalWrite(OUTPUT_PIN_1, HIGH);
    delay(3000);
    digitalWrite(OUTPUT_PIN_1, LOW);
    triggered = 1;

  } else if (sBuffer.indexOf("Leopard") != -1) {    // OUTPUT 2 is ultrasound and light
   // Serial.println("LEOPARD");
    digitalWrite(OUTPUT_PIN_2, HIGH); //
    delay(3000);
    digitalWrite(OUTPUT_PIN_2, LOW);
    triggered = 1;

  } else if (sBuffer.indexOf("Caracal") != -1) {    // OUTPUT 3 is just light
   // Serial.println("CARACAL");
    digitalWrite(OUTPUT_PIN_2, HIGH);
    delay(3000);
    digitalWrite(OUTPUT_PIN_2, LOW);
    triggered = 1;

  } else if (sBuffer.indexOf("Light") != -1) {
    //Serial.println("LIGHT");
    digitalWrite(OUTPUT_PIN_3, HIGH);   // This sets the light on for deterrent subsystem
    delay(3000);
    digitalWrite(OUTPUT_PIN_3, LOW);
    triggered = 1;                      // Still want image and timestamp for night-time

  }

  if (triggered) {
    sBuffer = "";
    struct tm now;
    if (getLocalTime(&now)) {
      char stamp[32];
      strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &now);
      sendToGoogleSheet(String(stamp));
    }
  }

  vTaskDelay(pdMS_TO_TICKS(20));  // Shorter delay for frequent checks
}

}

void sendToGoogleSheet(String timestamp) {
  HTTPClient http;
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.begin(scriptURL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  timestamp.replace(" ", "%20");
  timestamp.replace(":", "%3A");
  String postData = "timestamp=" + timestamp;
  Serial.println("Sending to sheet: " + postData);

  int code = http.POST(postData);
  
  if (code > 0) {
    Serial.println("POST status: " + String(code));

    // Only try to read the response if the code is 200, meaning successful
    if (code == HTTP_CODE_OK) {
      String response = http.getString();
      Serial.println("Response: " + response);
    } else {
      Serial.println("Non-200 response: " + String(code));
    }
  } else {
    Serial.println("POST request failed, error: " + http.errorToString(code));
  }

  http.end();
}

void displayTask(void* param) { //update screen with information. Most important part is the time updating every second
  while (true) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char buffer[32];
      strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
      if (xSemaphoreTake(screenSemaphore, portMAX_DELAY)) {
        u8g2.clearBuffer();
        u8g2.firstPage();
        do {
          writeScreen(buffer);
        } while (u8g2.nextPage());
        xSemaphoreGive(screenSemaphore);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));  // Update every second
  }
}

void networkTask(void* param) { //update the screen with information from the Google Sheet
  while (true) {
    readFromGoogleSheet();
    vTaskDelay(pdMS_TO_TICKS(10000)); // Every 10 seconds
  }
}

void setup() {
  Serial.begin(115200); //Debug line
  Serial2.begin(9600, SERIAL_8N1, UART_RX, UART_TX); // UART comms setup

  //Pin Setups for deterrent
  pinMode(OUTPUT_PIN_1, OUTPUT); //Honey badger
  pinMode(OUTPUT_PIN_2, OUTPUT); //Leopard and caracal
  pinMode(OUTPUT_PIN_3, OUTPUT); //Turn on LED

  digitalWrite(OUTPUT_PIN_1, LOW);
  digitalWrite(OUTPUT_PIN_2, LOW);
  digitalWrite(OUTPUT_PIN_3, LOW);

  screenSemaphore = xSemaphoreCreateBinary();
  xSemaphoreGive(screenSemaphore);

  initialiseScreen();
  connectWifi();
  readFromGoogleSheet();

  xTaskCreatePinnedToCore(displayTask, "Display Task", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(networkTask, "Network Task", 8192, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sendTrapTask, "Send Trap Instruction", 8192, NULL, 1, NULL, 1);
}

void loop() {
}
