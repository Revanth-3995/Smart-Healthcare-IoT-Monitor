#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SoftwareSerial.h>

// Software Serial pins: Pin 2 as RX (unused here), Pin 3 as TX (sends data to ESP32 RX Pin 18)
SoftwareSerial espSerial(2, 3); 
Adafruit_MPU6050 mpu;

void setup() {
  // Start hardware serial for USB debugging
  Serial.begin(115200);
  while (!Serial && millis() < 3000); 
  delay(1000);

  Serial.println("=========================================");
  Serial.println("MPU6050 Arduino Uno UART Sender Component");
  Serial.println("=========================================");

  // Start software serial to talk to ESP32
  espSerial.begin(115200);

  // Start hardware I2C bus (A4 = SDA, A5 = SCL on Arduino Uno)
  Wire.begin();

  if (!mpu.begin()) {
    Serial.println("[-] Error: MPU6050 sensor not found! Check wiring.");
    while (1) {
      delay(1000);
    }
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("[+] MPU6050 Initialized. Starting transmission...");
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Calculate Signal Vector Magnitude (SVM) in standard G forces
  float svm = sqrt(
    a.acceleration.x * a.acceleration.x +
    a.acceleration.y * a.acceleration.y +
    a.acceleration.z * a.acceleration.z
  ) / 9.81f;

  // Format CSV packet: ax,ay,az,gx,gy,gz,svm\n
  // Send data to ESP32 via SoftwareSerial TX
  espSerial.print(a.acceleration.x, 4); espSerial.print(",");
  espSerial.print(a.acceleration.y, 4); espSerial.print(",");
  espSerial.print(a.acceleration.z, 4); espSerial.print(",");
  espSerial.print(g.gyro.x, 4);          espSerial.print(",");
  espSerial.print(g.gyro.y, 4);          espSerial.print(",");
  espSerial.print(g.gyro.z, 4);          espSerial.print(",");
  espSerial.print(svm, 4);             espSerial.println();

  // Also print to hardware serial for local monitoring/debugging
  Serial.print("Sent: ");
  Serial.print(a.acceleration.x, 2); Serial.print(",");
  Serial.print(a.acceleration.y, 2); Serial.print(",");
  Serial.print(a.acceleration.z, 2); Serial.print(" | ");
  Serial.print(g.gyro.x, 2);          Serial.print(",");
  Serial.print(g.gyro.y, 2);          Serial.print(",");
  Serial.print(g.gyro.z, 2);          Serial.print(" | SVM: ");
  Serial.println(svm, 3);

  // Send packet every 100 milliseconds
  delay(100);
}
