#define TRIG_PIN 4
#define ECHO_PIN 5

#define MAX_DISTANCE_CM 300.0f
#define TIMEOUT_US      18000

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  Serial.println();
  Serial.println("==================================");
  Serial.println("HC-SR04 OPEN SPACE DIAGNOSTIC");
  Serial.println("==================================");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(TRIG_PIN, LOW);
}

void loop() {

  // Trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);

  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(TRIG_PIN, LOW);

  // Read echo
  long duration = pulseIn(ECHO_PIN, HIGH, TIMEOUT_US);

  Serial.println("--------------------------------");

  // No echo received
  if (duration == 0) {

    Serial.println("[TIMEOUT] No Echo Received");
    Serial.print("Distance      : ");
    Serial.print(MAX_DISTANCE_CM);
    Serial.println(" cm");

    Serial.println("[INFO] Open space / no reflection detected");

    delay(500);
    return;
  }

  // Suspiciously short pulse
  if (duration < 100) {

    Serial.print("[WARNING] False Echo Suspected : ");
    Serial.print(duration);
    Serial.println(" us");

    Serial.println("Distance      : INVALID");

    delay(500);
    return;
  }

  // Normal distance calculation
  float distance = duration * 0.0343f / 2.0f;

  Serial.print("Pulse Duration: ");
  Serial.print(duration);
  Serial.println(" us");

  Serial.print("Distance      : ");
  Serial.print(distance, 1);
  Serial.println(" cm");

  if (distance < 2.0f) {
    Serial.println("[WARNING] Below minimum measurable range");
  }
  else if (distance > 300.0f) {
    Serial.println("[WARNING] Above maximum measurable range");
  }
  else {
    Serial.println("[OK] Valid Reading");
  }

  delay(500);
}