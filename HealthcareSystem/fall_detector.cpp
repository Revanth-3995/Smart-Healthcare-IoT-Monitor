#include "fall_detector.h"

FallDetector::FallDetector() :
    _impactDetected(false),
    _lowPosture(false),
    _immobile(false),
    _fallDetected(false),
    _lastMotionTime(0)
{}

void FallDetector::update(const SensorData &data) {
    unsigned long now = millis();

    // 1. Impact detection: SVM > threshold
    if (data.svm > FALL_THRESHOLD_SVM) {
        _impactDetected = true;
    }

    // 2. Low posture detection: Distance < threshold
    if (data.distance_cm < FALL_THRESHOLD_DIST) {
        _lowPosture = true;
    } else {
        _lowPosture = false;
    }

    // 3. Immobility detection: No PIR motion for configurable duration
    if (data.pir_motion) {
        _lastMotionTime = now;
        _immobile = false;
    } else {
        if (now - _lastMotionTime >= IMMOBILE_DURATION_MS) {
            _immobile = true;
        }
    }

    // 4. Combined Fall detection
    _fallDetected = _impactDetected && _lowPosture && _immobile;
}

void FallDetector::reset() {
    _impactDetected = false;
    _lowPosture = false;
    _immobile = false;
    _fallDetected = false;
    _lastMotionTime = millis();
}
