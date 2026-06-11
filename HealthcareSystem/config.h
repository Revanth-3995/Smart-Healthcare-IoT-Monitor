#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// PIN CONFIGURATION (ESP32-S3 N16R8)
// ============================================================================
#define MPU_RX_PIN 18
#define MPU_TX_PIN 17
#define MPU_UART_BAUD 115200

#define ULTRASONIC_TRIG_PIN 4
#define ULTRASONIC_ECHO_PIN 5

#define PIR_PIN 45
#define IR_PIN 39

// ============================================================================
// ULTRASONIC SENSOR FILTERING & OPTIMIZATION
// ============================================================================
#define ULTRASONIC_TIMEOUT_US         18000    // Reduce blocking timeout to 18ms (~3m max range)
#define ULTRASONIC_EMA_ALPHA          0.25f    // Exponential Moving Average weight (0.25 = smooth)
#define ULTRASONIC_MIN_DURATION_US    100      // Reject echoes shorter than 100 microseconds
#define ULTRASONIC_MIN_VALID_CM       2.0f     // Minimum realistic distance
#define ULTRASONIC_MAX_VALID_CM       300.0f   // Maximum relevant monitoring range
#define ULTRASONIC_DEBUG              0        // Set to 1 to enable debug serial prints, 0 to disable

// ============================================================================
// ULTRASONIC ALERT ZONES CONFIGURATION
// ============================================================================
#define ULTRASONIC_PROXIMITY_MIN_CM  20.0f    // Too close warning (Zone 1)
#define ULTRASONIC_BED_EDGE_CM       80.0f    // Sitting up / at bed edge (Zone 2)
#define ULTRASONIC_ABSENT_CM        150.0f    // Patient out of bed (Zone 3)
#define ULTRASONIC_ALERT_DURATION_MS  3000    // Must persist 3s before alerting
#define ULTRASONIC_ALERT_COOLDOWN_MS 30000    // Don't repeat same alert for 30s

// ============================================================================
// WIFI & MQTT CONFIGURATION
// ============================================================================
#define WIFI_SSID     "Vivo..."
#define WIFI_PASSWORD "Nishu...."
#define MQTT_BROKER   "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "ESP32_Healthcare_Node"

// ============================================================================
// FALL DETECTION CONFIGURED THRESHOLDS
// ============================================================================
const float FALL_THRESHOLD_SVM = 2.50f;          // SVM g-force impact threshold
const float FALL_THRESHOLD_DIST = 30.0f;         // Low posture distance threshold (cm)
const unsigned long IMMOBILE_DURATION_MS = 10000; // Duration of no PIR motion to declare immobility (10s)

// ============================================================================
// SYSTEM INTERVALS (Milliseconds)
// ============================================================================
const unsigned long INTERVAL_SENSOR_SAMPLE = 100; // Telemetry & print interval (100ms)

#endif // CONFIG_H
