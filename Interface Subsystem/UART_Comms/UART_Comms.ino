#define TX 17  // GPIO 17 as TX
#define RX 16  // GPIO 16 as RX

void setup() {
  // Serial for USB debugging
  Serial.begin(115200);
  while (!Serial) {
    delay(10); // Wait for USB Serial to initialize
  }

  // Serial2 for UART communication
  Serial2.begin(9600, SERIAL_8N1, RX, TX);

  Serial.println("Serial Reader Ready.");
}

void loop() {
  // Check if data is available on Serial2 (UART)
  if (Serial2.available()) {
    // Read incoming message until newline
    String received = Serial2.readStringUntil('\n');
    received.trim(); // Remove \r, \n, spaces

    // Print it to the USB serial monitor
    Serial.print("Received: ");
    Serial.println(received);
  }
}
