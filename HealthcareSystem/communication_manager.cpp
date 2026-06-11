#include "communication_manager.h"

CommunicationManager::CommunicationManager() :
    _wifiConnected(false),
    _mqttConnected(false),
    _mqttClient(_wifiClient),
    _lastMqttReconnectAttempt(0),
    _lastProximityAlertTime(0),
    _lastAbsentAlertTime(0),
    _lastBedEdgeAlertTime(0),
    _distanceAlertStartTime(0),
    _lastDistanceAlertZone(0)
{}

bool CommunicationManager::begin() {
    Serial.print("[CommunicationManager] Connecting to WiFi SSID: ");
    Serial.println(WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < 5000) {
        delay(200);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        _wifiConnected = true;
        Serial.print("[CommunicationManager] WiFi Connected. IP Address: ");
        Serial.println(WiFi.localIP());
    } else {
        _wifiConnected = false;
        Serial.println("[CommunicationManager] WiFi connection failed or timed out. Auto-reconnect will keep retrying.");
    }

    _mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
    reconnectMqtt();

    return isConnected();
}

void CommunicationManager::update() {
    if (WiFi.status() != WL_CONNECTED) {
        _wifiConnected = false;
        _mqttConnected = false;
        return;
    }
    _wifiConnected = true;

    if (!_mqttClient.connected()) {
        _mqttConnected = false;
        unsigned long now = millis();
        if (now - _lastMqttReconnectAttempt > 5000) {
            _lastMqttReconnectAttempt = now;
            reconnectMqtt();
        }
    } else {
        _mqttConnected = true;
        _mqttClient.loop();
    }
}

void CommunicationManager::reconnectMqtt() {
    if (!_wifiConnected) return;

    Serial.println("[CommunicationManager] Connecting to MQTT Broker...");
    if (_mqttClient.connect(MQTT_CLIENT_ID)) {
        Serial.println("[CommunicationManager] MQTT Connected.");
        _mqttConnected = true;
    } else {
        Serial.print("[CommunicationManager] MQTT failed, state = ");
        Serial.println(_mqttClient.state());
        _mqttConnected = false;
    }
}

bool CommunicationManager::publishTelemetry(const SensorData &data, bool fallState, int predictedClass, float confidence, int activityScore) {
    if (!isConnected()) {
        return false;
    }

    char payload[256];

    // 1. Publish to healthcare/mpu
    snprintf(payload, sizeof(payload),
        "{\"accel_x\":%.3f,\"accel_y\":%.3f,\"accel_z\":%.3f,\"gyro_x\":%.3f,\"gyro_y\":%.3f,\"gyro_z\":%.3f,\"svm\":%.3f}",
        data.accel_x, data.accel_y, data.accel_z, data.gyro_x, data.gyro_y, data.gyro_z, data.svm);
    _mqttClient.publish("healthcare/mpu", payload);

    // 2. Publish to healthcare/distance
    snprintf(payload, sizeof(payload), "{\"distance_cm\":%.2f}", data.distance_cm);
    _mqttClient.publish("healthcare/distance", payload);

    // 3. Publish to healthcare/pir
    snprintf(payload, sizeof(payload), "{\"pir_motion\":%s}", data.pir_motion ? "true" : "false");
    _mqttClient.publish("healthcare/pir", payload);

    // 4. Publish to healthcare/ir
    snprintf(payload, sizeof(payload), "{\"ir_obstacle\":%s}", data.ir_obstacle ? "true" : "false");
    _mqttClient.publish("healthcare/ir", payload);

    // 5. Publish to healthcare/fall
    snprintf(payload, sizeof(payload), "{\"fall_detected\":%s}", fallState ? "true" : "false");
    _mqttClient.publish("healthcare/fall", payload);

    // 6. Publish to healthcare/prediction
    const char* classLabels[5] = {"Normal", "Resting", "Bed Exit", "Inactive", "Fall"};
    const char* labelStr = (predictedClass >= 0 && predictedClass < 5) ? classLabels[predictedClass] : "Unknown";
    snprintf(payload, sizeof(payload), "{\"label\":\"%s\",\"confidence\":%.1f,\"score\":%d}",
             labelStr, confidence * 100.0f, activityScore);
    _mqttClient.publish("healthcare/prediction", payload);

    // 7. Publish to healthcare/activityscore
    snprintf(payload, sizeof(payload), "%d", activityScore);
    _mqttClient.publish("healthcare/activityscore", payload);

    // 8. Publish to healthcare/alerts
    if (predictedClass == 4) {
        snprintf(payload, sizeof(payload), "{\"alert\":\"FALL DETECTED\",\"severity\":\"CRITICAL\"}");
        _mqttClient.publish("healthcare/alerts", payload);
    } else if (predictedClass == 3) {
        snprintf(payload, sizeof(payload), "{\"alert\":\"INACTIVITY\",\"severity\":\"WARNING\"}");
        _mqttClient.publish("healthcare/alerts", payload);
    } else if (predictedClass == 2) {
        snprintf(payload, sizeof(payload), "{\"alert\":\"BED EXIT\",\"severity\":\"INFO\"}");
        _mqttClient.publish("healthcare/alerts", payload);
    }

    // 9. Rule-based distance alert zones detection
    int currentZone = 0;
    if (data.distance_cm < ULTRASONIC_PROXIMITY_MIN_CM) {
        currentZone = 1; // Proximity
    } else if (data.distance_cm >= ULTRASONIC_ABSENT_CM) {
        currentZone = 3; // Absent
    } else if (data.distance_cm >= ULTRASONIC_BED_EDGE_CM) {
        currentZone = 2; // Bed Edge
    } else {
        currentZone = 0; // Normal
    }

    // Reset duration timer if zone changes
    if (currentZone != _lastDistanceAlertZone) {
        _lastDistanceAlertZone = currentZone;
        _distanceAlertStartTime = millis();
    } else if (currentZone != 0) {
        // If the same non-normal zone is active, check if duration condition has been met (3 seconds)
        if (millis() - _distanceAlertStartTime >= ULTRASONIC_ALERT_DURATION_MS) {
            unsigned long now = millis();
            if (currentZone == 1) { // Proximity
                // Do NOT fire proximity alert if an active fall event is detected
                if (!fallState) {
                    if (now - _lastProximityAlertTime >= ULTRASONIC_ALERT_COOLDOWN_MS) {
                        snprintf(payload, sizeof(payload), "{\"alert\":\"PROXIMITY WARNING\",\"severity\":\"INFO\",\"distance_cm\":%.1f}", data.distance_cm);
                        _mqttClient.publish("healthcare/alerts", payload);
                        _lastProximityAlertTime = now;
                    }
                }
            } else if (currentZone == 2) { // Bed Edge
                if (now - _lastBedEdgeAlertTime >= ULTRASONIC_ALERT_COOLDOWN_MS) {
                    snprintf(payload, sizeof(payload), "{\"alert\":\"BED EDGE DETECTED\",\"severity\":\"INFO\",\"distance_cm\":%.1f}", data.distance_cm);
                    _mqttClient.publish("healthcare/alerts", payload);
                    _lastBedEdgeAlertTime = now;
                }
            } else if (currentZone == 3) { // Absent
                if (now - _lastAbsentAlertTime >= ULTRASONIC_ALERT_COOLDOWN_MS) {
                    snprintf(payload, sizeof(payload), "{\"alert\":\"PATIENT ABSENT\",\"severity\":\"WARNING\",\"distance_cm\":%.1f}", data.distance_cm);
                    _mqttClient.publish("healthcare/alerts", payload);
                    _lastAbsentAlertTime = now;
                }
            }
        }
    }

    return true;
}
