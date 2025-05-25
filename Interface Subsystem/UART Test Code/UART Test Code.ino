#define TX1 17
#define RX1 16  // Not used in this sketch

const char* messages[] = {"Honey", "Leopard", "Caracal", "Light"};
unsigned long lastSend = 0;

void setup() {
  Serial.begin(115200);  // USB monitor
  Serial1.begin(9600, SERIAL_8N1, RX1, TX1);  // UART for TX only
  Serial.println("UART Companion Sender Ready.");
}

void loop() {
  if (millis() - lastSend > 2000) {
    String msg = messages[random(0, 4)];
    Serial1.println(msg);  // Send message
    Serial.print("Sent to other device: ");
    Serial.println(msg);
    lastSend = millis();
  }
}
