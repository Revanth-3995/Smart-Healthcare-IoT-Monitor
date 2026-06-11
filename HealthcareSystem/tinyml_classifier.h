#ifndef TINYML_CLASSIFIER_H
#define TINYML_CLASSIFIER_H

#define FLATBUFFERS_USE_STD_SPAN

#include <Arduino.h>
#include "model.h"
#include "scaler_params.h"
#include "sensor_manager.h"

// TensorFlow Lite Micro standard headers for ESP32
#include <TensorFlowLite_ESP32.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/schema/schema_generated.h>

class TinyMLClassifier {
public:
    TinyMLClassifier() : 
        _model(nullptr),
        _interpreter(nullptr), 
        _input(nullptr), 
        _output(nullptr), 
        _activityScore(50), 
        _lastPirMotionTime(0),
        _lastPirPenaltyTime(0),
        _initialized(false) {}

    bool begin() {
        // Map the model into a usable data structure
        _model = tflite::GetModel(g_model);
        if (_model->version() != TFLITE_SCHEMA_VERSION) {
            Serial.println("[TinyML] ERROR: Model schema version mismatch!");
            return false;
        }

        // Pull in all the operation implementations
        static tflite::AllOpsResolver resolver;

        // Build an interpreter to run the model with
        static tflite::MicroInterpreter static_interpreter(
            _model, resolver, _tensorArena, kTensorArenaSize, tflite::GetMicroErrorReporter());
        _interpreter = &static_interpreter;

        // Allocate memory from the tensor_arena for the model's tensors
        TfLiteStatus allocate_status = _interpreter->AllocateTensors();
        if (allocate_status != kTfLiteOk) {
            Serial.println("[TinyML] ERROR: AllocateTensors() failed!");
            return false;
        }

        // Obtain pointers to the model's input and output tensors
        _input = _interpreter->input(0);
        _output = _interpreter->output(0);

        _lastPirMotionTime = millis();
        _lastPirPenaltyTime = millis();
        _initialized = true;
        
        Serial.println("[TinyML] Model loaded OK");
        return true;
    }

    // Runs inference and returns the predicted class (0-4), updates the confidence and activityScore
    int classify(const SensorData &data, float &confidence, bool impactDetected) {
        if (!_initialized) {
            confidence = 0.0f;
            return -1;
        }

        // 1. Arrange input features in the exact expected order:
        // Index 0-2: accel_x, accel_y, accel_z (in g)
        // Index 3-5: gyro_x, gyro_y, gyro_z (in deg/s)
        // Index 6: distance_cm
        // Index 7: pir_motion (0.0 or 1.0)
        // Index 8: ir_obstacle (0.0 or 1.0)
        float rawFeatures[9] = {
            data.accel_x / 9.80665f,                      // Convert m/s^2 to g
            data.accel_y / 9.80665f,
            data.accel_z / 9.80665f,
            data.gyro_x * 57.2957795f,                    // Convert rad/s to deg/s
            data.gyro_y * 57.2957795f,
            data.gyro_z * 57.2957795f,
            data.distance_cm,
            data.pir_motion ? 1.0f : 0.0f,
            data.ir_obstacle ? 1.0f : 0.0f
        };

        // 2. Apply StandardScaler normalization: (raw - mean) / std
        for (int i = 0; i < 9; i++) {
            _input->data.f[i] = (rawFeatures[i] - SCALER_MEAN[i]) / SCALER_STD[i];
        }

        // 3. Run inference
        TfLiteStatus invoke_status = _interpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            Serial.println("[TinyML] ERROR: Inference Invoke() failed!");
            confidence = 0.0f;
            return -1;
        }

        // 4. Find the class with the highest probability
        int predictedClass = 0;
        float maxProbability = _output->data.f[0];
        for (int i = 1; i < 5; i++) {
            if (_output->data.f[i] > maxProbability) {
                maxProbability = _output->data.f[i];
                predictedClass = i;
            }
        }
        confidence = maxProbability;

        // Apply fusion safety rule: A Fall (class 4) requires a physical impact detection.
        // If no impact was detected, override the Fall prediction to Inactive (class 3) if low posture, or Resting (class 1) otherwise.
        if (predictedClass == 4 && !impactDetected) {
            if (data.distance_cm < 30.0f) {
                predictedClass = 3; // Inactive (Laying down/Asleep)
            } else {
                predictedClass = 1; // Resting (Sitting/Standing)
            }
            confidence = _output->data.f[predictedClass];
        }

        // 5. Update Activity Score:
        // +2 for Normal, -1 for Resting, -3 for Inactive, -20 for Fall
        // -5 if no PIR motion for 5+ minutes
        unsigned long now = millis();
        if (data.pir_motion) {
            _lastPirMotionTime = now;
        }

        int scoreChange = 0;
        if (predictedClass == 0) {
            scoreChange += 2;
        } else if (predictedClass == 1) {
            scoreChange -= 1;
        } else if (predictedClass == 3) {
            scoreChange -= 3;
        } else if (predictedClass == 4) {
            scoreChange -= 20;
        }

        // Handle the Inactivity penalty: -5 points every 5 seconds after 5 minutes of no PIR motion
        if (now - _lastPirMotionTime >= 300000) {
            if (now - _lastPirPenaltyTime >= 5000) {
                scoreChange -= 5;
                _lastPirPenaltyTime = now;
            }
        } else {
            // Keep penalty timer aligned when PIR is active
            _lastPirPenaltyTime = now;
        }

        _activityScore += scoreChange;
        
        // Clamp the activity score between 0 and 100
        if (_activityScore > 100) _activityScore = 100;
        if (_activityScore < 0)   _activityScore = 0;

        return predictedClass;
    }

    int getActivityScore() const { return _activityScore; }
    bool isInitialized() const { return _initialized; }

private:
    const tflite::Model* _model;
    tflite::MicroInterpreter* _interpreter;
    TfLiteTensor* _input;
    TfLiteTensor* _output;
    
    int _activityScore;
    unsigned long _lastPirMotionTime;
    unsigned long _lastPirPenaltyTime;
    bool _initialized;

    // Define the tensor arena size (8KB is sufficient for this simple 3-layer MLP)
    static const int kTensorArenaSize = 8 * 1024;
    uint8_t _tensorArena[kTensorArenaSize] __attribute__((aligned(16)));
};

#endif // TINYML_CLASSIFIER_H
