#include "esp_camera.h"
#include "Arduino.h"
#include "driver/rtc_io.h"

#define LED_PIN 4

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.print("Start");
  delay(1000);

  pinMode(LED_PIN, OUTPUT);
  pinMode(GPIO_NUM_13, INPUT_PULLUP); //commented out when i added the gpio pin 13 and sleep config

  Serial.print("Switching LED ON");
  digitalWrite(LED_PIN, true);
  delay(1000);
  digitalWrite(LED_PIN, false);

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_13, 0); 
  Serial.println("Sleeping");
  esp_deep_sleep_start();
} 
 
void loop() {
  
}