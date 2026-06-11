#ifndef FALL_DETECTOR_H
#define FALL_DETECTOR_H

#include "config.h"
#include "sensor_manager.h"

class FallDetector {
public:
    FallDetector();
    void update(const SensorData &data);
    void reset();

    bool isImpactDetected() const { return _impactDetected; }
    bool isLowPosture() const { return _lowPosture; }
    bool isImmobile() const { return _immobile; }
    bool isFallDetected() const { return _fallDetected; }

private:
    bool _impactDetected;
    bool _lowPosture;
    bool _immobile;
    bool _fallDetected;

    unsigned long _lastMotionTime;
};

#endif // FALL_DETECTOR_H
