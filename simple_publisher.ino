#include <WiFi.h>
#include <PubSubClient.h>

// WiFi configuration
const char* ssid = "xxxxxx";  // WiFi SSID
const char* password = "xxxxxx";  // WiFi password

// MQTT Broker (mqtt.eclipseprojects.io Public Broker）
const char* mqtt_server = "mqtt.eclipseprojects.io";  
const int mqtt_port = 1883;  // MQTT default port

WiFiClient espClient;
PubSubClient client(espClient);

// Connect WiFi
void setup_wifi() {
    delay(10);
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected!");
}

// Connect MQTT Broker
void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("ESP32S3_Client")) {
            Serial.println("Connected to MQTT broker!");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(client.state());
            Serial.println(" Trying again in 5 seconds...");
            delay(5000);
        }
    }
}

void setup() {
    Serial.begin(115200);
    setup_wifi();
    
    client.setServer(mqtt_server, mqtt_port);
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    Serial.println("Publishing MQTT message...");
    client.publish("test/topic", "hello from ESP32S3");
    
    delay(5000);  // Publish every 5 seconds
}
