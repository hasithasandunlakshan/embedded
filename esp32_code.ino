/*
 * IoT Environmental Monitoring System
 * University of Moratuwa EN2853 Programming Assignment 2
 * Author: Hasitha Sandun
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include "DHTesp.h"
#include <ESP32Servo.h>

// WiFi credentials
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// MQTT Broker settings
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_client_id = "esp32_hasithasandun";

// MQTT Topics
const char* topic_temp = "hasithasandun/temperature";
const char* topic_humid = "hasithasandun/humidity";
const char* topic_light = "hasithasandun/light";
const char* topic_servo = "hasithasandun/servo";
const char* topic_alert = "hasithasandun/alert";
const char* topic_servo_control = "hasithasandun/servo_control";
const char* topic_temp_threshold = "hasithasandun/temp_threshold";

// Pin definitions
const int DHT_PIN = 15;       // DHT22 data pin
const int LDR_PIN = 34;       // Light sensor analog pin
const int BUZZER_PIN = 12;    // Buzzer pin
const int SERVO_PIN = 26;     // Servo motor pin

// Threshold values
float temp_threshold = 30.0;  // Temperature threshold for alert (can be changed via MQTT)
int light_threshold = 2000;   // Light threshold for servo control

// Variables
float temperature = 0.0;
float humidity = 0.0;
int lightLevel = 0;
bool autoMode = true;         // Servo control mode (auto/manual)
int servoAngle = 0;

// Timing variables
unsigned long lastSensorReadTime = 0;
unsigned long lastMQTTPublishTime = 0;
const long sensorReadInterval = 2000;    // Read sensors every 2 seconds
const long mqttPublishInterval = 5000;   // Publish to MQTT every 5 seconds

// Object instances
WiFiClient espClient;
PubSubClient client(espClient);
DHTesp dht;
Servo servo;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("IoT Environmental Monitoring System");
  Serial.println("Author: Hasitha Sandun");
  
  // Initialize pins
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LDR_PIN, INPUT);
  
  // Initialize DHT sensor
  dht.setup(DHT_PIN, DHTesp::DHT22);
  
  // Initialize servo
  ESP32PWM::allocateTimer(0);
  servo.setPeriodHertz(50);    // Standard 50Hz servo
  servo.attach(SERVO_PIN, 500, 2400);  // Attach servo with min/max pulse width
  servo.write(0);  // Initial position
  
  // Connect to WiFi
  setupWiFi();
  
  // Connect to MQTT broker
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
}

void loop() {
  // Ensure WiFi and MQTT connection
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Read sensors at regular intervals
  unsigned long currentMillis = millis();
  if (currentMillis - lastSensorReadTime >= sensorReadInterval) {
    lastSensorReadTime = currentMillis;
    readSensors();
    
    // Check if temperature exceeds threshold for alert
    if (temperature > temp_threshold) {
      triggerAlert();
    } else {
      digitalWrite(BUZZER_PIN, LOW); // Turn off buzzer
    }
    
    // Control servo based on mode (auto/manual)
    if (autoMode) {
      controlServoAuto();
    }
  }
  
  // Publish to MQTT at regular intervals
  if (currentMillis - lastMQTTPublishTime >= mqttPublishInterval) {
    lastMQTTPublishTime = currentMillis;
    publishData();
  }
}

// Read sensor values
void readSensors() {
  // Read temperature and humidity
  TempAndHumidity data = dht.getTempAndHumidity();
  temperature = data.temperature;
  humidity = data.humidity;
  
  // Read light level
  lightLevel = analogRead(LDR_PIN);
  
  // Print values to serial for debugging
  Serial.println("Sensor Readings:");
  Serial.print("Temperature: "); Serial.print(temperature); Serial.println(" °C");
  Serial.print("Humidity: "); Serial.print(humidity); Serial.println(" %");
  Serial.print("Light Level: "); Serial.println(lightLevel);
}

// Publish data to MQTT broker
void publishData() {
  // Convert values to strings
  char tempStr[8];
  char humidStr[8];
  char lightStr[8];
  char servoStr[8];
  
  dtostrf(temperature, 1, 2, tempStr);
  dtostrf(humidity, 1, 2, humidStr);
  itoa(lightLevel, lightStr, 10);
  itoa(servoAngle, servoStr, 10);
  
  // Publish to respective topics
  client.publish(topic_temp, tempStr);
  client.publish(topic_humid, humidStr);
  client.publish(topic_light, lightStr);
  client.publish(topic_servo, servoStr);
  
  Serial.println("Data published to MQTT broker");
}

// Setup WiFi connection
void setupWiFi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

// Reconnect to MQTT broker
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(mqtt_client_id)) {
      Serial.println("connected");
      
      // Subscribe to topics for incoming commands
      client.subscribe(topic_servo_control);
      client.subscribe(topic_temp_threshold);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" trying again in 5 seconds");
      delay(5000);
    }
  }
}

// MQTT message callback
void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
  payload[length] = '\0';
  String message = String((char*)payload);
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  Serial.println(message);
  
  // Handle servo control messages
  if (String(topic) == topic_servo_control) {
    if (message.startsWith("AUTO")) {
      autoMode = true;
      Serial.println("Servo control mode set to AUTO");
    } else if (message.startsWith("MANUAL")) {
      autoMode = false;
      int angle = message.substring(6).toInt();
      if (angle >= 0 && angle <= 180) {
        servoAngle = angle;
        servo.write(servoAngle);
        Serial.print("Servo manually set to angle: ");
        Serial.println(servoAngle);
      }
    }
  }
  
  // Handle temperature threshold messages
  if (String(topic) == topic_temp_threshold) {
    float newThreshold = message.toFloat();
    if (newThreshold > 0 && newThreshold < 100) {
      temp_threshold = newThreshold;
      Serial.print("Temperature threshold updated to: ");
      Serial.println(temp_threshold);
    }
  }
}

// Control servo automatically based on light level
void controlServoAuto() {
  // Map light level to servo angle (inverse relationship)
  int newAngle = map(lightLevel, 0, 4095, 180, 0);
  
  // Only update if angle changed significantly to avoid jitter
  if (abs(newAngle - servoAngle) > 5) {
    servoAngle = newAngle;
    servo.write(servoAngle);
    Serial.print("Auto servo adjustment to angle: ");
    Serial.println(servoAngle);
  }
}

// Trigger alert when temperature exceeds threshold
void triggerAlert() {
  digitalWrite(BUZZER_PIN, HIGH);
  Serial.println("ALERT: Temperature threshold exceeded!");
  
  // Send alert message to MQTT
  char alertMsg[50];
  sprintf(alertMsg, "Temperature Alert: %.2f°C exceeds threshold %.2f°C", temperature, temp_threshold);
  client.publish(topic_alert, alertMsg);
}