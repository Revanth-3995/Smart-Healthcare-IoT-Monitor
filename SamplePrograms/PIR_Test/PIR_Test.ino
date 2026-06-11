#define PIR_PIN 45

int lastPirState = -1;
bool wasConnected = false;

// Helper function to check if the PIR sensor is physically connected.
// It detects whether the pin is floating (disconnected) by switching between
// internal pull-up and pull-down modes and observing if the level changes.
bool isPIRConnected() {
  // 1. Enable internal pull-up and read
  pinMode(PIR_PIN, INPUT_PULLUP);
  delayMicroseconds(100); // Allow pin voltage to settle
  int valPullUp = digitalRead(PIR_PIN);

  // 2. Enable internal pull-down and read
  pinMode(PIR_PIN, INPUT_PULLDOWN);
  delayMicroseconds(100); // Allow pin voltage to settle
  int valPullDown = digitalRead(PIR_PIN);

  // Restore the normal operating pin mode
  pinMode(PIR_PIN, INPUT_PULLDOWN);

  // If the pin voltage follows the internal pull-up/down resistors,
  // it means the pin is floating (disconnected).
  // If the external sensor is connected, its output driver will override 
  // the weak internal pulls, resulting in the same reading in both cases.
  if (valPullUp != valPullDown) {
    return false; // Floating / Disconnected
  }
  return true; // Driven HIGH or LOW / Connected
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("PIR Sensor Connection & Component Test");
  Serial.println("=========================================");

  // Initialize with standard input pull-down
  pinMode(PIR_PIN, INPUT_PULLDOWN);

  // Initial connection check
  wasConnected = isPIRConnected();
  Serial.print("[PIR] Initial Connection Status: ");
  Serial.println(wasConnected ? "CONNECTED" : "DISCONNECTED (Check wiring!)");
}

void loop() {
  bool currentlyConnected = isPIRConnected();

  // Print connection status transitions
  if (currentlyConnected != wasConnected) {
    wasConnected = currentlyConnected;
    Serial.println("----------------------------------------");
    Serial.print("[!] CONNECTION CHANGE: PIR is now ");
    Serial.println(currentlyConnected ? "CONNECTED" : "DISCONNECTED");
  }

  if (currentlyConnected) {
    int currentPirState = digitalRead(PIR_PIN);

    // Print state when motion detection transitions
    if (currentPirState != lastPirState) {
      Serial.println("----------------------------------------");
      Serial.print("PIR Pin Level : "); Serial.println(currentPirState == HIGH ? "HIGH" : "LOW");
      Serial.print("Motion State  : ");
      if (currentPirState == HIGH) {
        Serial.println("MOTION DETECTED");
      } else {
        Serial.println("NO MOTION");
      }
      lastPirState = currentPirState;
    }
  } else {
    // Force state reset when disconnected
    lastPirState = -1;
  }

  delay(200); // Sample connection and state changes every 200 ms
}
