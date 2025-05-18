#include <HTTPClient.h>
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
const char* scriptURL = "https://script.google.com/macros/s/AKfycbxJqB3kqn8MRBNbBCDFvmSqqBCqxBBuJPeWDvLEfh6rRCa0mIYv6NrRi-SD4b77jeZT/exec";

// UART pins
#define UART_TX 17
#define UART_RX 16

// To trap //need 12 13 14
#define OUTPUT_PIN_1 12
#define OUTPUT_PIN_2 13
#define OUTPUT_PIN_3 14

// Data
String captureTimestamps[3] = { "--", "--", "--" };
String unitNumber = "--";
String sBuffer = "";
SemaphoreHandle_t screenSemaphore;

void initialiseScreen() {
  Serial.println("=== Screen Init ===");
  Serial.println("Connecting WiFi...");
  Serial.println("===================");
}

void connectWifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  configTime(3600 * 2, 0, "pool.ntp.org", "time.nist.gov");
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo)) {
    Serial.println("Waiting for time sync...");
    delay(1000);
  }
}

void writeScreen(const char* currentTime) {
  Serial.println();
  Serial.println("==== Display Output ====");
  Serial.println(String("Time: ") + currentTime);
  Serial.println(String("Unit: ") + unitNumber);
  Serial.println("-------------------------");
  Serial.println("Recent Captures:");
  Serial.println("1. " + captureTimestamps[2]);
  Serial.println("2. " + captureTimestamps[1]);
  Serial.println("3. " + captureTimestamps[0]);
  Serial.println("=========================");
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

void sendTrapTask(void* param) { //MODIFIED FOR USER INPUT - this is fine because it reads every 1 second
    while (true) {
      if (Serial2.available()) {

        char c = Serial2.read();
        
        if (c >= 32 && c <= 126) { sBuffer += c;} //add to buffer
        if (sBuffer.length() > 10) {sBuffer = sBuffer.substring(sBuffer.length() - 10);} //limit to last 10 chars

        Serial.println(String("Buffer: ") + sBuffer);


        if (sBuffer.indexOf("HB") != -1) {
          Serial.println("HONEY BADGER");
          digitalWrite(OUTPUT_PIN_1, HIGH);
          delay(1000);
          digitalWrite(OUTPUT_PIN_1, LOW);
          sBuffer = "";
          
      } else if (sBuffer.indexOf("LD") != -1) {
          Serial.println("LEOPARD");
          digitalWrite(OUTPUT_PIN_2, HIGH);
          delay(1000);
          digitalWrite(OUTPUT_PIN_2, LOW);
          sBuffer = "";

      } else if (sBuffer.indexOf("CL") != -1) {
          Serial.println("CARACAL");
          digitalWrite(OUTPUT_PIN_3, HIGH);
          delay(1000);
          digitalWrite(OUTPUT_PIN_3, LOW);
          sBuffer = "";

      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
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

  int code = http.POST(postData);                       //delete for final?
  Serial.println("POST status: " + String(code));
  Serial.println("Response: " + http.getString()); 

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
        //writeScreen(buffer);
        Serial.print(".");
        xSemaphoreGive(screenSemaphore);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void uartNotifyTask(void* param) {
  while (true) {
    if (Serial2.available()) {
      String msg = Serial2.readStringUntil('\n');
      msg.trim();
      if (msg.startsWith("ERROR:")) {
        Serial.println("[UART ERROR] " + msg);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(15000));
  }
}

void networkTask(void* param) {
  while (true) {
    readFromGoogleSheet();
    vTaskDelay(pdMS_TO_TICKS(15000));
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, UART_RX, UART_TX); 

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
  xTaskCreatePinnedToCore(sendTrapTask, "Send Trap Instruction", 2048, NULL, 1, NULL, 1);
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
