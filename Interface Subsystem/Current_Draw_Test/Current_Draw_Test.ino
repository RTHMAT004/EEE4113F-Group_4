#define OUTPUT_PIN 12
String input = "";

void setup() {
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);  // Start LOW
  Serial.begin(115200);
  while (!Serial) {
    delay(10);
  }
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();

    if (c == '\n' || c == '\r') {
      input.trim();  // Remove trailing whitespace or newlines

      if (input.equalsIgnoreCase("ON")) {
        Serial.println("GPIO12 → HIGH for 4 seconds");
        digitalWrite(OUTPUT_PIN, HIGH);
        delay(4000);  // Stay HIGH for 4 seconds
        digitalWrite(OUTPUT_PIN, LOW);
        Serial.println("GPIO12 → LOW");
      } else {
        Serial.println("Unrecognized command. Type ON to trigger output.");
      }

      input = "";  // Clear input buffer
    } else {
      input += c;  // Build the input string character by character
    }
  }
}

