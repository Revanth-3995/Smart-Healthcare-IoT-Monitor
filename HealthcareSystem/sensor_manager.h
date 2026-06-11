#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include "config.h"
// No direct MPU I2C library needed on ESP32 anymore. We read from Serial1.
#include <Wire.h>

// Unified sensor data structure
struct SensorData {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float svm;
    float distance_cm;
    bool pir_motion;
    bool ir_obstacle;
};

class SensorManager {
public:
    SensorManager();
    bool begin();
    void update();

    // Get current unified sensor data structure
    SensorData getData() const { return _data; }
    
    // Get individual values
    float getSVM() const { return _data.svm; }
    float getDistance() const { return _data.distance_cm; }
    bool getPIRMotion() const { return _data.pir_motion; }
    bool isMPUConnected() const { return _mpuConnected; }

private:
    bool _mpuConnected;
    SensorData _data;
    float _lastValidDistance;

    char _serialBuffer[128];
    int _serialBufferIndex;
    unsigned long _lastPacketTime;
    unsigned long _lastPirHighTime;
    void parseArduinoData(const char* buffer);

    float readDistance();
};

#endif // SENSOR_MANAGER_H
