#define TRIG_PIN 4
#define ECHO_PIN 5

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(1000);

  Serial.println("==================================");
  Serial.println("HC-SR04 Individual Component Test");
  Serial.println("==================================");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Set trigger to low initially
  digitalWrite(TRIG_PIN, LOW);
  
  Serial.println("[+] Ultrasonic Sensor Pins Configured. Measuring distance...");
}

void loop() {
  // Clear trigger pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);

  // Send 10 microsecond pulse to trigger pin
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Read the echo pin pulse duration (maximum 30ms timeout = approx 5 meters)
  long duration = pulseIn(ECHO_PIN, HIGH, 30000);

  Serial.println("--------------------------------");
  if (duration == 0) {
    Serial.println("[-] Error: No Echo Detected (Timeout). Check power and wiring.");
  } else {
    // Calculate distance in centimeters
    float distance = (float)duration * 0.0343 / 2.0;
    
    Serial.print("Pulse Duration: "); Serial.print(duration); Serial.println(" us");
    Serial.print("Distance      : "); Serial.print(distance, 1); Serial.println(" cm");
    
    // Add logic filter preview
    if (distance < 2.0 || distance > 300.0) {
      Serial.println("[!] Status    : OUT OF RANGE (2cm - 300cm filter will ignore this)");
    } else {
      Serial.println("[+] Status    : VALID RANGE");
    }
  }

  delay(500);
}
