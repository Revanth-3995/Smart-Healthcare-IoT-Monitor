#define IR_PIN 16

bool wasConnected = false;
int lastObstacleState = -1;

// Helper to check if the IR sensor is physically connected.
// Switches between internal pull-up and pull-down configurations.
// If the reading changes, the pin is floating (disconnected).
bool isIRConnected() {
  pinMode(IR_PIN, INPUT_PULLUP);
  delayMicroseconds(100);
  int valPullUp = digitalRead(IR_PIN);

  pinMode(IR_PIN, INPUT_PULLDOWN);
  delayMicroseconds(100);
  int valPullDown = digitalRead(IR_PIN);

  pinMode(IR_PIN, INPUT); // Default to floating input or weak pull-up/down

  if (valPullUp != valPullDown) {
    return false; // Disconnected / Floating
  }
  return true; // Connected
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(1000);

  Serial.println("=========================================");
  Serial.println("IR Obstacle Sensor Component Test");
  Serial.println("=========================================");

  // Check initial connection
  wasConnected = isIRConnected();
  Serial.print("[IR] Connection Status: ");
  Serial.println(wasConnected ? "CONNECTED" : "DISCONNECTED (Check wiring!)");
}

void loop() {
  bool currentlyConnected = isIRConnected();

  if (currentlyConnected != wasConnected) {
    wasConnected = currentlyConnected;
    Serial.println("----------------------------------------");
    Serial.print("[!] CONNECTION CHANGE: IR Sensor is now ");
    Serial.println(currentlyConnected ? "CONNECTED" : "DISCONNECTED");
  }

  if (currentlyConnected) {
    // Standard IR obstacle sensors output LOW when an obstacle is detected,
    // and HIGH when the path is clear.
    int pinLevel = digitalRead(IR_PIN);
    bool obstacleDetected = (pinLevel == LOW);

    if (obstacleDetected != lastObstacleState) {
      Serial.println("----------------------------------------");
      Serial.print("IR Pin Level    : "); Serial.println(pinLevel == HIGH ? "HIGH" : "LOW");
      Serial.print("Obstacle Status : ");
      if (obstacleDetected) {
        Serial.println("OBSTACLE DETECTED");
      } else {
        Serial.println("CLEAR / NO OBSTACLE");
      }
      lastObstacleState = obstacleDetected;
    }
  } else {
    lastObstacleState = -1;
  }

  delay(100);
}
