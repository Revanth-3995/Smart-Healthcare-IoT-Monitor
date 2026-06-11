#include "sensor_manager.h"

SensorManager::SensorManager() :
    _mpuConnected(false),
    _lastValidDistance(150.0f),
    _serialBufferIndex(0),
    _lastPacketTime(0),
    _lastPirHighTime(0)
{
    _data.accel_x = 0.0f;
    _data.accel_y = 0.0f;
    _data.accel_z = 0.0f;
    _data.gyro_x = 0.0f;
    _data.gyro_y = 0.0f;
    _data.gyro_z = 0.0f;
    _data.svm = 1.0f;
    _data.distance_cm = 150.0f;
    _data.pir_motion = false;
    _data.ir_obstacle = false;
    memset(_serialBuffer, 0, sizeof(_serialBuffer));
}

bool SensorManager::begin() {
    // Initialize pins for HC-SR04
    pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
    pinMode(ULTRASONIC_ECHO_PIN, INPUT);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    // Initialize PIR pin
    pinMode(PIR_PIN, INPUT_PULLDOWN);

    // Initialize IR sensor pin
    pinMode(IR_PIN, INPUT);

    // Initialize UART connection to Arduino Uno
    Serial.println("[SensorManager] Initializing UART connection to Arduino Uno...");
    Serial1.begin(MPU_UART_BAUD, SERIAL_8N1, MPU_RX_PIN, MPU_TX_PIN);

    _mpuConnected = false;
    _lastPacketTime = millis();

    return true;
}

void SensorManager::update() {
    // Diagnostic: Check for electrical transitions on the RX pin (GPIO 16)
    static unsigned long lastPinTransition = 0;
    static bool lastPinState = HIGH;
    static unsigned long lastDiagnosticTime = 0;

    bool currentPinState = digitalRead(MPU_RX_PIN);
    if (currentPinState != lastPinState) {
        lastPinState = currentPinState;
        lastPinTransition = millis();
    }

    // 1. Read UART data from Arduino Uno
    while (Serial1.available() > 0) {
        char c = Serial1.read();
        if (c == '\n') {
            _serialBuffer[_serialBufferIndex] = '\0';
            parseArduinoData(_serialBuffer);
            _serialBufferIndex = 0;
        } else if (c == '\r') {
            // Ignore carriage return
        } else if (_serialBufferIndex < (int)sizeof(_serialBuffer) - 1) {
            _serialBuffer[_serialBufferIndex++] = c;
        }
    }

    // Check for serial communication timeout (2 seconds)
    if (_mpuConnected && (millis() - _lastPacketTime > 2000)) {
        _mpuConnected = false;
        Serial.println("[SensorManager] ERROR: MPU6050 data timeout from Arduino Uno!");
    }

    if (!_mpuConnected) {
        _data.accel_x = 0.0f;
        _data.accel_y = 0.0f;
        _data.accel_z = 0.0f;
        _data.gyro_x = 0.0f;
        _data.gyro_y = 0.0f;
        _data.gyro_z = 0.0f;
        _data.svm = 1.0f;

        // Print physical diagnostics to Serial Monitor every 5 seconds
        if (millis() - lastDiagnosticTime > 5000) {
            lastDiagnosticTime = millis();
            if (millis() - lastPinTransition < 1000) {
                Serial.print("[UART Diagnostic] ⚠️ Physical signal activity detected on RX Pin ");
                Serial.print(MPU_RX_PIN);
                Serial.println(", but the ESP32 hardware UART cannot decode the bytes. This is due to SoftwareSerial bit-timing corruption on the Arduino Uno at 115200 baud.");
            } else {
                Serial.print("[UART Diagnostic] ❌ No electrical signals detected on RX Pin ");
                Serial.print(MPU_RX_PIN);
                Serial.print(". Verify that Arduino Pin 3 (TX) is wired to ESP32 Pin ");
                Serial.print(MPU_RX_PIN);
                Serial.println(" (RX) and both boards share a common GND wire.");
            }
        }
    }

    // 2. Read HC-SR04
    float rawDistance = readDistance();
    if (rawDistance >= 2.0f && rawDistance <= 300.0f) {
        _data.distance_cm = rawDistance;
        _lastValidDistance = rawDistance;
    } else {
        // Ignore outlier and reuse last valid distance
        _data.distance_cm = _lastValidDistance;
    }

    // 3. Read PIR State (with software hold debouncer of 5 seconds)
    bool rawPir = (digitalRead(PIR_PIN) == HIGH);
    
    // Ignore PIR during the first 60 seconds (PIR sensor physical calibration & warm-up phase)
    if (millis() < 60000) {
        rawPir = false;
        _lastPirHighTime = 0;
    }
 
    if (rawPir) {
        _lastPirHighTime = millis();
        _data.pir_motion = true;
    } else {
        if (_lastPirHighTime == 0 || millis() - _lastPirHighTime >= 5000) {
            _data.pir_motion = false;
        } else {
            _data.pir_motion = true; // Hold active state
        }
    }

    // 4. Read IR Obstacle State (Active LOW logic: LOW = obstacle detected)
    _data.ir_obstacle = (digitalRead(IR_PIN) == LOW);
}

void SensorManager::parseArduinoData(const char* buffer) {
    // If we receive a packet, we know some serial data is coming in
    float ax, ay, az, gx, gy, gz, svm;
    int parsed = sscanf(buffer, "%f,%f,%f,%f,%f,%f,%f", &ax, &ay, &az, &gx, &gy, &gz, &svm);
    if (parsed == 7) {
        _data.accel_x = ax;
        _data.accel_y = ay;
        _data.accel_z = az;
        _data.gyro_x = gx;
        _data.gyro_y = gy;
        _data.gyro_z = gz;
        _data.svm = svm;
        _mpuConnected = true;
        _lastPacketTime = millis();
    } else {
        // Log the corrupt line to the console for debugging
        Serial.print("[SensorManager] Warning: Received malformed packet (parsed ");
        Serial.print(parsed);
        Serial.print("/7 values). Raw content: \"");
        Serial.print(buffer);
        Serial.println("\"");
    }
}

float SensorManager::readDistance() {
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(ULTRASONIC_TRIG_PIN, LOW);

    long duration = pulseIn(ULTRASONIC_ECHO_PIN, HIGH, 30000);
    if (duration == 0) {
        return -1.0f;
    }
    return (float)duration * 0.0343f / 2.0f;
}
