/**********************************************************
 * ESP32-side code: Send IR camera data in chunks (120 bytes each)
 * Compile/Upload in Arduino IDE
 **********************************************************/
#include <WiFi.h>
#include <PubSubClient.h>
#include <base64.h> // Arduino Base64 library header file (example)

// ------------ Modify here: Your WiFi & MQTT information ------------
const char* ssid = "xxxx";
const char* password = "xxxx";
const char* mqtt_server = "mqtt.eclipseprojects.io";
const int   mqtt_port   = 1883;

// ------------ IR Image Information ------------
static const int IR_WIDTH = 32;
static const int IR_HEIGHT = 24;
static const int IR_DATA_SIZE = IR_WIDTH * IR_HEIGHT; // 768 bytes (8-bit)
uint8_t irFrame[IR_DATA_SIZE];  // Store infrared image data

// Chunk size
static const int CHUNK_SIZE = 120;  
// Total number of chunks (the last chunk may be less than 120 bytes)
static const int TOTAL_CHUNKS = (IR_DATA_SIZE + CHUNK_SIZE - 1) / CHUNK_SIZE;

// WiFi & MQTT setup
WiFiClient espClient;
PubSubClient client(espClient);

// ================== Callback function (if message reception is needed, implement here) ==================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

// ================== Connect to WiFi ==================
void setup_wifi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

// ================== Connect to MQTT ==================
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32_IR_";
    clientId += String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // If subscription to a topic is needed, add here:
      // client.subscribe("some/subscribe/topic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// ================== Fill simulated IR data ==================
void fillIRData() {
  // Example only: Fill with cyclic values from 0~255 as "dummy data"
  for (int i = 0; i < IR_DATA_SIZE; i++) {
    irFrame[i] = i % 256;
  }
}

// ================== Send IR data in chunks (Base64 + JSON) ==================
void sendIRDataInChunks() {
  // Send index = 0 ~ (TOTAL_CHUNKS - 1)
  int offset = 0;
  for (int i = 0; i < TOTAL_CHUNKS; i++) {
    int bytesToSend = min(CHUNK_SIZE, IR_DATA_SIZE - offset);

    // 1) Extract the binary data of this chunk
    uint8_t* chunkPtr = irFrame + offset;

    // 2) Encode in Base64
    String encoded = base64::encode(chunkPtr, bytesToSend);

    // 3) Assemble into a JSON string, including index / total / size / base64 data
    //    Example: {"index":0,"total":7,"size":120,"data":"..."}
    char payloadBuf[512];
    snprintf(payloadBuf, sizeof(payloadBuf),
      "{\"index\":%d,\"total\":%d,\"size\":%d,\"data\":\"%s\"}",
      i, TOTAL_CHUNKS, bytesToSend, encoded.c_str()
    );

    // 4) Publish to MQTT
    boolean result = client.publish("esp32/ir/chunk", payloadBuf);
    if (result) {
      Serial.print("Published chunk #");
      Serial.print(i);
      Serial.print(" offset=");
      Serial.print(offset);
      Serial.print(" bytes=");
      Serial.print(bytesToSend);
      Serial.print(" Base64_len=");
      Serial.println(encoded.length());
    } else {
      Serial.println("Failed to publish chunk!");
    }

    offset += bytesToSend;
    // To prevent blocking due to sending too fast, introduce a slight delay
    delay(50);
  }

  Serial.println("All chunks published!");
}

// ================== Arduino setup() ==================
void setup() {
  Serial.begin(115200);

  // Fill simulated IR data
  fillIRData();

  // Connect to WiFi
  setup_wifi();

  // MQTT setup
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

// ================== Arduino loop() ==================
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Example: Send a full frame every 15 seconds
  static unsigned long lastSend = 0;
  unsigned long now = millis();
  if (now - lastSend > 15000) {
    lastSend = now;
    Serial.println("Sending IR data in chunks...");
    sendIRDataInChunks();
  }
}
