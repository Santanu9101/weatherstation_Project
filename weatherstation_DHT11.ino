#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>

const char* ssid = "iQOO Z7 Pro 5G";         
const char* password = "Santanu@04";   
const char* mqtt_server = "10.150.22.54"; 
const char* topic = "sensor/value";     

const int mq135Pin = D0;                
const int LDR_pin = A0;                 
#define DHTPIN D2  
#define MSG_BUFFER_SIZE (200)
char json[MSG_BUFFER_SIZE];                      
DHT dht(DHTPIN, DHT11);                  
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  
  pinMode(mq135Pin, INPUT);              
  pinMode(LDR_pin, INPUT);               
  dht.begin();                           
 
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  
  client.setServer(mqtt_server, 1883); 
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  int mq135Value = analogRead(mq135Pin); 
  int ldrValue = analogRead(LDR_pin);
  float temperature = dht.readTemperature();   
  float humidity = dht.readHumidity();         

 
  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }

  
  StaticJsonDocument<300> doc;
  JsonObject root = doc.to<JsonObject>();
  root["Air_Quality"] = mq135Value;  // Use the correct variable for Air Quality
  root["LDR"] = ldrValue;            // Use the correct variable for LDR
  root["Temperature"] = temperature; // Use the correct variable for Temperature
  root["Humidity"] = humidity;      // Use the correct variable for Humidity

  
  char json[MSG_BUFFER_SIZE];
  serializeJson(doc, json);

  
  Serial.print("Publishing message: ");
  Serial.println(json);

  // Publish the JSON message to the MQTT topic
  client.publish(topic, json);

  delay(2000);  // Wait for 2 seconds before publishing again
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    if (client.connect("NodeMCUClient")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      delay(2000);  // Wait 2 seconds before trying again
    }
  }
}
