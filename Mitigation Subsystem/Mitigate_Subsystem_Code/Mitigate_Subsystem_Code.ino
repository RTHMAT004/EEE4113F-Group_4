const int analogPin1 = A0;  // Light
const int analogPin2 = A1;  // HB
const int analogPin3 = A2;  // Cat

const int threshold = 614;  // Approx. 3V threshold

const int pin5 = 5;  // LED
const int pin6 = 6;  // Siren
const int pin7 = 7;  // Ultrasound

enum Mode { NONE, LIGHT, HB, CAT };
Mode currentMode = NONE;

void setup() {
  pinMode(pin5, OUTPUT);
  pinMode(pin6, OUTPUT);
  pinMode(pin7, OUTPUT);
}

void loop() {
  int val1 = analogRead(analogPin1);
  int val2 = analogRead(analogPin2);
  int val3 = analogRead(analogPin3);

  // Exit mode if its input goes low
  switch (currentMode) {
    case LIGHT:
      if (val1 < threshold) {
        digitalWrite(pin5, LOW);
        currentMode = NONE;
      }
      break;
    case HB:
      if (val2 < threshold) {
        digitalWrite(pin5, LOW);
        digitalWrite(pin6, LOW);
        currentMode = NONE;
      }
      break;
    case CAT:
      if (val3 < threshold) {
        digitalWrite(pin5, LOW);
        digitalWrite(pin7, LOW);
        currentMode = NONE;
      }
      break;
    case NONE:
      break;
  }

  // If no active mode, check for new input
  if (currentMode == NONE) {
    if (val3 >= threshold) currentMode = CAT;
    else if (val2 >= threshold) currentMode = HB;
    else if (val1 >= threshold) currentMode = LIGHT;
  }

  // Perform actions depending on current mode
  switch (currentMode) {
    case LIGHT:
      digitalWrite(pin5, HIGH);  // LED solid on
      break;

    case HB:
      digitalWrite(pin6, HIGH);  // Siren on
      digitalWrite(pin5, HIGH); delay(100);
      digitalWrite(pin5, LOW); delay(100);
      break;

    case CAT:
      digitalWrite(pin7, HIGH);  // Ultrasound on
      digitalWrite(pin5, HIGH); delay(100);
      digitalWrite(pin5, LOW); delay(100);
      break;

    case NONE:
      delay(10);  // idle delay
      break;
  }
}
