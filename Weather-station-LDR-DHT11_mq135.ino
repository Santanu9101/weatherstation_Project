#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>
#include <WiFiManager.h>

#define DHTPIN D2  
#define DHTTYPE DHT11
#define MQ135_PIN D0
#define LDR_PIN A0
#define MSG_BUFFER_SIZE 200

char mqtt_server[40] = "broker.hivemq.com"; // Default MQTT broker
char mqtt_port[6] = "1883";                // Default port
char topic[40] = "sensor/value";           // Default topic

DHT dht(DHTPIN, DHTTYPE);
WiFiClient espClient;
PubSubClient client(espClient);

bool shouldSaveConfig = false;
unsigned long lastPublishTime = 0;         // Timer for non-blocking delay
const unsigned long publishInterval = 2000; // 2 seconds

void saveConfigCallback() {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  pinMode(MQ135_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);

  // WiFiManager setup
  WiFiManager wifiManager;

  WiFiManagerParameter custom_mqtt_server("server", "MQTT Server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT Port", mqtt_port, 6);
  WiFiManagerParameter custom_topic("topic", "MQTT Topic", topic, 40);

  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_topic);
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  wifiManager.autoConnect("santanu weather station");

  // Save updated configuration
  strcpy(mqtt_server, custom_mqtt_server.getValue());
  strcpy(mqtt_port, custom_mqtt_port.getValue());
  strcpy(topic, custom_topic.getValue());

  Serial.println("WiFi connected!");
  Serial.println("IP Address: " + WiFi.localIP().toString());
  Serial.println("MQTT Server: " + String(mqtt_server));
  Serial.println("MQTT Port: " + String(mqtt_port));
  Serial.println("MQTT Topic: " + String(topic));

  client.setServer(mqtt_server, atoi(mqtt_port));
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Publish data at intervals
  unsigned long currentMillis = millis();
  if (currentMillis - lastPublishTime >= publishInterval) {
    publishSensorData();
    lastPublishTime = currentMillis;
  }
}

void publishSensorData() {
  int mq135Value = analogRead(MQ135_PIN);
  int ldrValue = analogRead(LDR_PIN);
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  // Prepare JSON payload
  StaticJsonDocument<300> doc;
  doc["Air_Quality"] = mq135Value;
  doc["LDR"] = ldrValue;
  doc["Temperature"] = temperature;
  doc["Humidity"] = humidity;

  char json[MSG_BUFFER_SIZE];
  serializeJson(doc, json);

  Serial.print("Publishing message: ");
  Serial.println(json);

  // Publish to MQTT topic
  if (client.publish(topic, json)) {
    Serial.println("Message published successfully!");
  } else {
    Serial.println("Failed to publish message!");
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.println(client.state());
      delay(2000); // Retry every 2 seconds
    }
  }
}