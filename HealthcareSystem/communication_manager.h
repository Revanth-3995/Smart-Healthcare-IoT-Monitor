#ifndef COMMUNICATION_MANAGER_H
#define COMMUNICATION_MANAGER_H

#include "config.h"
#include "sensor_manager.h"
#include <WiFi.h>
#include <PubSubClient.h>

class CommunicationManager {
public:
    CommunicationManager();
    bool begin();
    void update();
    bool publishTelemetry(const SensorData &data, bool fallState, int predictedClass, float confidence, int activityScore);

    bool isConnected() const { return _wifiConnected && _mqttConnected; }

private:
    WiFiClient _wifiClient;
    PubSubClient _mqttClient;
    bool _wifiConnected;
    bool _mqttConnected;
    unsigned long _lastMqttReconnectAttempt;

    // Ultrasonic alert zone state variables
    unsigned long _lastProximityAlertTime;
    unsigned long _lastAbsentAlertTime;
    unsigned long _lastBedEdgeAlertTime;
    unsigned long _distanceAlertStartTime;
    int _lastDistanceAlertZone; // (0=normal, 1=proximity, 2=bed edge, 3=absent)

    void reconnectMqtt();
};

#endif // COMMUNICATION_MANAGER_H
