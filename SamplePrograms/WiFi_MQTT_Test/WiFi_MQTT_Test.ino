#include <WiFi.h>
#include <PubSubClient.h>

// WiFi Settings - Replace with your credentials
#define WIFI_SSID     "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// MQTT Settings - Replace with your broker details
#define MQTT_BROKER   "192.168.1.100"
#define MQTT_PORT     1883
#define MQTT_CLIENT_ID "ESP32_WiFi_MQTT_Test_Client"

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastPublishTime = 0;
unsigned int publishCount = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);
  delay(1000);

  Serial.println("==================================");
  Serial.println("WiFi & MQTT Network Component Test");
  Serial.println("==================================");

  // Connect to WiFi
  Serial.print("[WiFi] Connecting to SSID: ");
  Serial.println(WIFI_SSID);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startMs < 10000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[+] WiFi Connected successfully!");
    Serial.print("[WiFi] IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[-] Error: WiFi Connection failed or timed out. Check credentials/SSID signal.");
  }

  // Set MQTT server details
  client.setServer(MQTT_BROKER, MQTT_PORT);
  
  reconnectMqtt();
}

void reconnectMqtt() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[MQTT] Cannot attempt connection, WiFi is disconnected.");
    return;
  }

  Serial.print("[MQTT] Connecting to broker ");
  Serial.print(MQTT_BROKER);
  Serial.print(" on port ");
  Serial.println(MQTT_PORT);

  if (client.connect(MQTT_CLIENT_ID)) {
    Serial.println("[+] MQTT Broker Connected successfully!");
  } else {
    Serial.print("[-] Error: MQTT broker connection failed, state = ");
    Serial.println(client.state());
    Serial.println("[MQTT] Hint: -1 = connection cleared, -2 = connect failed, -3 = connection lost, -4 = connect timeout");
  }
}

void loop() {
  // Maintain WiFi and MQTT connection
  if (WiFi.status() != WL_CONNECTED) {
    static unsigned long lastWifiCheck = 0;
    if (millis() - lastWifiCheck > 10000) {
      Serial.println("[WiFi] Reconnect attempt initiated...");
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      lastWifiCheck = millis();
    }
    return;
  }

  if (!client.connected()) {
    unsigned long now = millis();
    if (now - lastPublishTime > 5000) {
      reconnectMqtt();
      lastPublishTime = now;
    }
    return;
  }

  client.loop();

  // Publish heartbeat payload every 3 seconds
  unsigned long now = millis();
  if (now - lastPublishTime >= 3000) {
    publishCount++;
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"heartbeat_count\":%u,\"uptime\":%lu,\"rssi\":%d}", publishCount, now, WiFi.RSSI());
    
    Serial.println("--------------------------------");
    Serial.print("[MQTT] Publishing test payload: ");
    Serial.println(payload);
    
    if (client.publish("healthcare/system/test", payload)) {
      Serial.println("[+] Topic published successfully!");
    } else {
      Serial.println("[-] Error: Publish failed!");
    }
    
    lastPublishTime = now;
  }
}
