#include <Arduino.h>

// Reuse pin configuration and baud rate from config.h or define them directly here
#define MPU_RX_PIN 18
#define MPU_TX_PIN 17
#define MPU_UART_BAUD 115200

char serialBuffer[128];
int serialBufferIndex = 0;
unsigned long lastPacketTime = 0;
bool mpuConnected = false;

// Variables to store parsed readings
float accel_x, accel_y, accel_z;
float gyro_x, gyro_y, gyro_z;
float svm;

void parseArduinoData(const char* buffer) {
  float ax, ay, az, gx, gy, gz, s;
  int parsed = sscanf(buffer, "%f,%f,%f,%f,%f,%f,%f", &ax, &ay, &az, &gx, &gy, &gz, &s);
  if (parsed == 7) {
    accel_x = ax;
    accel_y = ay;
    accel_z = az;
    gyro_x = gx;
    gyro_y = gy;
    gyro_z = gz;
    svm = s;
    mpuConnected = true;
    lastPacketTime = millis();
    
    // Print success to USB Serial
    Serial.println("----------------------------------------");
    Serial.print("[Parser] Accel: "); Serial.print(accel_x, 2); Serial.print(", "); Serial.print(accel_y, 2); Serial.print(", "); Serial.println(accel_z, 2);
    Serial.print("[Parser] Gyro : "); Serial.print(gyro_x, 2);  Serial.print(", "); Serial.print(gyro_y, 2);  Serial.print(", "); Serial.println(gyro_z, 2);
    Serial.print("[Parser] SVM  : "); Serial.println(svm, 3);
  } else {
    Serial.print("[Parser] ERROR: Failed parsing packet: ");
    Serial.println(buffer);
  }
}

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("=========================================");
  Serial.println("MPU6050 ESP32-S3 UART Receiver Component");
  Serial.println("=========================================");

  // Initialize hardware serial 1 with custom pins
  Serial1.begin(MPU_UART_BAUD, SERIAL_8N1, MPU_RX_PIN, MPU_TX_PIN);
  
  Serial.print("[+] Serial1 receiver started at baud ");
  Serial.println(MPU_UART_BAUD);
  Serial.print("[+] Pins -> RX: GPIO"); Serial.print(MPU_RX_PIN);
  Serial.print(", TX: GPIO"); Serial.println(MPU_TX_PIN);
  
  lastPacketTime = millis();
}

void loop() {
  // Read incoming characters from Serial1
  while (Serial1.available() > 0) {
    char c = Serial1.read();
    if (c == '\n') {
      serialBuffer[serialBufferIndex] = '\0';
      parseArduinoData(serialBuffer);
      serialBufferIndex = 0;
    } else if (c == '\r') {
      // Ignore carriage returns
    } else if (serialBufferIndex < (int)sizeof(serialBuffer) - 1) {
      serialBuffer[serialBufferIndex++] = c;
    }
  }

  // Check for communication timeout (2 seconds)
  if (mpuConnected && (millis() - lastPacketTime > 2000)) {
    mpuConnected = false;
    Serial.println("[-] WARNING: MPU6050 data timeout! Check connection or Arduino Uno power.");
  }

  delay(10);
}
