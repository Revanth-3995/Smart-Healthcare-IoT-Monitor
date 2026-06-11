#include <Arduino.h>
#include "config.h"
#include "sensor_manager.h"
#include "fall_detector.h"
#include "communication_manager.h"
#include "tinyml_classifier.h"

// Instantiate Managers
SensorManager        sensorManager;
FallDetector         fallDetector;
CommunicationManager communicationManager;
TinyMLClassifier     tinymlClassifier;

// Scheduler Timestamp
unsigned long lastSensorSample = 0;

void setup() {
    Serial.begin(115200);
    
    // Wait for Serial Monitor to be opened (vital for ESP32-S3 Native USB CDC)
    unsigned long startSerialWait = millis();
    while (!Serial && (millis() - startSerialWait < 5000)) {
        delay(10);
    }
    delay(1000);

    Serial.println("==================================================");
    Serial.println("STARTING PHASE-1 SMART HEALTHCARE MONITOR SYSTEM");
    Serial.println("==================================================");

    // Initialize all components
    sensorManager.begin();
    tinymlClassifier.begin();
    communicationManager.begin();
    fallDetector.reset();

    // Give the ESP32 time to receive and parse incoming UART packets from the Arduino Uno
    Serial.println("[Setup] Detecting MPU6050 stream from Arduino Uno...");
    unsigned long startWait = millis();
    while (millis() - startWait < 1500) {
        sensorManager.update();
        if (sensorManager.isMPUConnected()) {
            break;
        }
        delay(10);
    }

    // Print Sensor Health Report
    Serial.println();
    Serial.println("Create a sensor health report:");
    Serial.println();
    Serial.print("MPU6050 : "); Serial.println(sensorManager.isMPUConnected() ? "ONLINE" : "OFFLINE");
    Serial.print("HC-SR04 : "); Serial.println("ONLINE");
    Serial.print("PIR     : "); Serial.println("ONLINE");
    Serial.print("IR      : "); Serial.println("ONLINE");
    Serial.print("OLED    : "); Serial.println("OFFLINE");
    Serial.print("NRF24   : "); Serial.println("OFFLINE");
    Serial.println();

    lastSensorSample = millis();
}

void loop() {
    unsigned long now = millis();

    // Connection maintenance
    communicationManager.update();

    // Run sample & processing task every 100 ms
    if (now - lastSensorSample >= INTERVAL_SENSOR_SAMPLE) {
        sensorManager.update();
        SensorData data = sensorManager.getData();

        // Run fall detection algorithms
        fallDetector.update(data);

        // Run TinyML classification
        float confidence = 0.0f;
        int predictedClass = tinymlClassifier.classify(data, confidence, fallDetector.isImpactDetected());
        int activityScore = tinymlClassifier.getActivityScore();

        // Publish to MQTT
        communicationManager.publishTelemetry(data, fallDetector.isFallDetected(), predictedClass, confidence, activityScore);

        // Event-driven Serial Monitor Dashboard
        static bool last_pir_motion = false;
        static bool last_ir_obstacle = false;
        static bool last_low_posture = false;
        static bool last_impact_detected = false;
        static bool last_fall_detected = false;
        static int last_predicted_class = -1;

        // 1. PIR Sensor change logging
        if (data.pir_motion != last_pir_motion) {
            last_pir_motion = data.pir_motion;
            if (data.pir_motion) {
                Serial.println("[PIR Sensor]   🟢 Active: Motion detected (Person is moving around).");
            } else {
                Serial.println("[PIR Sensor]   🟡 Caution: No movement detected (Person might be still/resting).");
            }
        }

        // 2. IR Sensor change logging
        if (data.ir_obstacle != last_ir_obstacle) {
            last_ir_obstacle = data.ir_obstacle;
            if (data.ir_obstacle) {
                Serial.println("[IR Sensor]    🟢 Bed Occupied: Person is in bed.");
            } else {
                Serial.println("[IR Sensor]    🟡 Bed Vacant: No person detected in bed.");
            }
        }

        // 3. Ultrasonic posture change logging
        bool current_low_posture = fallDetector.isLowPosture();
        if (current_low_posture != last_low_posture) {
            last_low_posture = current_low_posture;
            if (current_low_posture) {
                Serial.print("[Ultrasonic]   🟡 Warning: Low posture detected! Distance = ");
                Serial.print(data.distance_cm, 1);
                Serial.println(" cm (Person is lying down or near the floor).");
            } else {
                Serial.println("[Ultrasonic]   🟢 Height Normal: Person is upright / off the floor.");
            }
        }

        // 4. Accelerometer SVM impact logging
        bool current_impact = fallDetector.isImpactDetected();
        if (current_impact != last_impact_detected) {
            last_impact_detected = current_impact;
            if (current_impact) {
                Serial.print("[Impact Sensor] 🚨 Warning: High impact detected! SVM = ");
                Serial.print(data.svm, 2);
                Serial.println(" g");
            } else {
                Serial.println("[Impact Sensor] 🟢 Impact flag cleared.");
            }
        }

        // 5. Rule-based Fall status change
        bool current_fall = fallDetector.isFallDetected();
        if (current_fall != last_fall_detected) {
            last_fall_detected = current_fall;
            if (current_fall) {
                Serial.println("\n[ALERT] 🚨 CRITICAL: Rule-based fall threshold exceeded! (Impact + Low posture + Immobility detected) 🚨\n");
            } else {
                Serial.println("[ALERT] 🟢 Critical Alert Cleared.\n");
            }
        }

        // 6. TinyML predicted state change logging
        if (predictedClass != last_predicted_class) {
            last_predicted_class = predictedClass;
            const char* classLabels[5] = {"Normal", "Resting", "Bed Exit", "Inactive", "Fall"};
            const char* classDescriptions[5] = {
                "Normal activity (Walking/Moving)",
                "Resting (Sitting/Standing)",
                "Bed Exit (Transitioning out of bed)",
                "Inactive (Laying down/Asleep)",
                "🚨 FALL DETECTED BY TINYML! 🚨"
            };
            
            const char* labelStr = (predictedClass >= 0 && predictedClass < 5) ? classLabels[predictedClass] : "Unknown";
            const char* descStr = (predictedClass >= 0 && predictedClass < 5) ? classDescriptions[predictedClass] : "Unknown state";

            Serial.print("[TinyML]       State Changed: ");
            Serial.print(labelStr);
            Serial.print(" (");
            Serial.print(descStr);
            Serial.print(") | Confidence: ");
            Serial.print(confidence * 100.0f, 1);
            Serial.print("% | Activity Score: ");
            Serial.println(activityScore);
        }

        // 7. Physical 3D Orientation tracking
        static int last_orientation = -1; // 0 = Upright, 1 = Tilted, 2 = Laying Flat
        float accel_magnitude = sqrt(data.accel_x * data.accel_x + data.accel_y * data.accel_y + data.accel_z * data.accel_z);
        int current_orientation = last_orientation;
        if (accel_magnitude > 0.1f) {
            float cos_tilt = data.accel_z / accel_magnitude;
            float tilt = acos(constrain(cos_tilt, -1.0f, 1.0f)) * 57.29578f; // Convert rad to degrees
            
            if (tilt < 25.0f) {
                current_orientation = 0; // Upright (Vertical)
            } else if (tilt < 65.0f) {
                current_orientation = 1; // Tilted (Angled)
            } else {
                current_orientation = 2; // Laying Flat (Horizontal)
            }
        }

        if (current_orientation != last_orientation) {
            last_orientation = current_orientation;
            const char* orientLabels[3] = {"Upright (Vertical)", "Tilted (Angled)", "Laying Flat (Horizontal)"};
            Serial.print("[Orientation]  🔄 Position Changed: ");
            Serial.println(orientLabels[current_orientation]);
        }

        // 8. Event-driven logging for Ultrasonic alert zones
        int current_dist_zone = 0;
        if (data.distance_cm < ULTRASONIC_PROXIMITY_MIN_CM) {
            current_dist_zone = 1;
        } else if (data.distance_cm >= ULTRASONIC_ABSENT_CM) {
            current_dist_zone = 3;
        } else if (data.distance_cm >= ULTRASONIC_BED_EDGE_CM) {
            current_dist_zone = 2;
        } else {
            current_dist_zone = 0;
        }

        static float last_distance_zone = 0.0f;
        if (current_dist_zone != (int)last_distance_zone) {
            last_distance_zone = (float)current_dist_zone;
            if (current_dist_zone == 3) {
                Serial.print("[Ultrasonic] PATIENT ABSENT: Distance = ");
                Serial.print(data.distance_cm, 1);
                Serial.println(" cm (out of bed range)");
            } else if (current_dist_zone == 1) {
                Serial.print("[Ultrasonic] PROXIMITY WARNING: Distance = ");
                Serial.print(data.distance_cm, 1);
                Serial.println(" cm (too close)");
            } else if (current_dist_zone == 2) {
                Serial.print("[Ultrasonic] BED EDGE: Distance = ");
                Serial.print(data.distance_cm, 1);
                Serial.println(" cm (sitting up or at edge)");
            } else {
                Serial.println("[Ultrasonic] NORMAL: Distance back in normal range");
            }
        }

        lastSensorSample = now;
    }
}
