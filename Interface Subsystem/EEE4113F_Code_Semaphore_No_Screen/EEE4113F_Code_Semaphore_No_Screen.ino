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

// UART pins
#define UART_TX 17
#define UART_RX 16

// To trap //need 12 13 14
#define OUTPUT_PIN_1 12
#define OUTPUT_PIN_2 13
#define OUTPUT_PIN_3 14

// Screen object
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
  // Serial.println();
  // Serial.println("==== Display Output ====");
  // Serial.println(String("Time: ") + currentTime);
  // Serial.println(String("Unit: ") + unitNumber);
  // Serial.println("-------------------------");
  // Serial.println("Recent Captures:");
  // Serial.println("1. " + captureTimestamps[2]);
  // Serial.println("2. " + captureTimestamps[1]);
  // Serial.println("3. " + captureTimestamps[0]);
  // Serial.println("=========================");

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
  //MODIFIED FOR USER INPUT - this is fine because it reads every 1 second
    // while (true) {
    //   if (Serial2.available()) {

    //     char c = Serial2.read();
        
    //     if (c >= 32 && c <= 126) { sBuffer += c;} //add to buffer
    //     if (sBuffer.length() > 10) {sBuffer = sBuffer.substring(sBuffer.length() - 10);} //limit to last 10 chars

    //     Serial.println(String("Buffer: ") + sBuffer);


    //     if (sBuffer.indexOf("Honey") != -1) {
    //       Serial.println("HONEY BADGER");
    //       digitalWrite(OUTPUT_PIN_1, HIGH);
    //       delay(1000);
    //       digitalWrite(OUTPUT_PIN_1, LOW);
    //       sBuffer = "";
          
    //   } else if (sBuffer.indexOf("Leopard") != -1) {
    //       Serial.println("LEOPARD");
    //       digitalWrite(OUTPUT_PIN_2, HIGH);
    //       delay(1000);
    //       digitalWrite(OUTPUT_PIN_2, LOW);
    //       sBuffer = "";

    //   } else if (sBuffer.indexOf("Caracal") != -1) {
    //       Serial.println("CARACAL");
    //       digitalWrite(OUTPUT_PIN_3, HIGH);
    //       delay(1000);
    //       digitalWrite(OUTPUT_PIN_3, LOW);
    //       sBuffer = "";

    //   }
    // }
    // vTaskDelay(pdMS_TO_TICKS(500)); //check trigger every 0.5s
    // }

//BELOW IS THE CORRECT CODE FOR THE FINAL DEMO
while (true) {
  while (Serial2.available()) {
    char c = Serial2.read();
    if (c >= 32 && c <= 126) {
      sBuffer += c;
    }
  }

  if (sBuffer.length() > 20) {
    sBuffer = sBuffer.substring(sBuffer.length() - 20);
  }

  Serial.println(String("Buffer: ") + sBuffer);


  int triggered = 0;

  if (sBuffer.indexOf("Honey") != -1) {
    Serial.println("HONEY BADGER");
    digitalWrite(OUTPUT_PIN_1, HIGH);
    delay(1000);
    digitalWrite(OUTPUT_PIN_1, LOW);
    triggered = 1;
  } else if (sBuffer.indexOf("Leopard") != -1) {
    Serial.println("LEOPARD");
    digitalWrite(OUTPUT_PIN_2, HIGH);
    delay(1000);
    digitalWrite(OUTPUT_PIN_2, LOW);
    triggered = 1;
  } else if (sBuffer.indexOf("Caracal") != -1) {
    Serial.println("CARACAL");
    digitalWrite(OUTPUT_PIN_3, HIGH);
    delay(1000);
    digitalWrite(OUTPUT_PIN_3, LOW);
    triggered = 1;
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

    // Optional: Only try to read the response if the code is 200
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

// void sendErrorToGoogleSheet(const String& errorMessage) {
//   HTTPClient http;
//   http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
//   http.begin(scriptURL);  // Same script URL
//   http.addHeader("Content-Type", "application/x-www-form-urlencoded");

//   String encodedMsg = errorMessage;
//   encodedMsg.replace(" ", "%20");
//   encodedMsg.replace(":", "%3A");

//   String postData = "error=" + encodedMsg;
//   Serial.println("Sending error: " + postData);

//   int code = http.POST(postData);
//   Serial.println("POST status: " + String(code));
//   Serial.println("Response: " + http.getString());

//   http.end();
// }

void displayTask(void* param) {
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

// void uartNotifyTask(void* param) {
//   while (true) {
//     if (Serial2.available()) {
//       String msg = Serial2.readStringUntil('\n');
//       msg.trim();
//       if (msg.startsWith("ERROR:")) {
//         Serial.println("[UART ERROR] " + msg);
//       }
//     }
//     vTaskDelay(pdMS_TO_TICKS(15000));
//   }
// }

void networkTask(void* param) {
  while (true) {
    readFromGoogleSheet();
    vTaskDelay(pdMS_TO_TICKS(10000)); // Every 10 seconds
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, UART_RX, UART_TX); // UART comms setup

  //Pin Setups
  pinMode(OUTPUT_PIN_1, OUTPUT);
  pinMode(OUTPUT_PIN_2, OUTPUT);
  pinMode(OUTPUT_PIN_3, OUTPUT);

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
 // xTaskCreatePinnedToCore(uartNotifyTask, "UART Notify Task", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(sendTrapTask, "Send Trap Instruction", 8192, NULL, 1, NULL, 1);
}

String cleanInput(String input) {
  String output = "";
  for (int i = 0; i < input.length(); i++) {
    char c = input.charAt(i);
    if (c >= 32 && c <= 126) {  // Only printable ASCII
      output += c;
    }
  }
  return output;
}


void loop() {
  // if (Serial.available()) {
  //   char c = Serial.read();
  //   if (c == 'T') {
  //     struct tm now;
  //     if (getLocalTime(&now)) {
  //       char stamp[32];
  //       strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", &now);
  //       sendToGoogleSheet(String(stamp));
  //     }
  //   }
  // }

//     if (Serial2.available()) {
//       String msg = Serial2.readStringUntil('\n');
//       msg.trim();  // Remove \r, spaces, etc.
//       msg = cleanInput(msg);
//       msg = msg.substring(msg.length()-2);
      
//   // Only allow 2-character commands
//     if (msg.length() == 2) {
//       Serial.print("Received: ");
//       Serial.println(msg);

//     if (msg == "HB") {
//       Serial.println("HONEY BADGER");
//       // digitalWrite(OUTPUT_PIN_1, HIGH);
//       // delay(1000);
//       // digitalWrite(OUTPUT_PIN_1, LOW);
//     } else if (msg == "LD") {
//       Serial.println("LEOPARD");
//       // digitalWrite(OUTPUT_PIN_2, HIGH);
//       // delay(1000);
//       // digitalWrite(OUTPUT_PIN_2, LOW);
//     } else if (msg == "CL") {
//       Serial.println("CARACAL");
//       // digitalWrite(OUTPUT_PIN_3, HIGH);
//       // delay(1000);
//       // digitalWrite(OUTPUT_PIN_3, LOW);
//     } else {
//       Serial.println("Unknown 2-letter code: " + msg);
//     }
//   } else {
//     // Ignore invalid or noisy strings
//     Serial.print("Ignored garbage/partial message: ");
//     Serial.println(msg);
//   }

//   // Optional: flush any leftover serial noise
//   while (Serial2.available()) Serial2.read();
// }
    //vTaskDelay(pdMS_TO_TICKS(1000));
}
