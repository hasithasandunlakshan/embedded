#include <WiFi.h> // For connecting to the internet
#include <PubSubClient.h> // For MQTT connectivity
#include "DHTesp.h"  // For interfacing with DHT temperature and humidity sensor
#include <WiFiUdp.h>
#include <ESP32Servo.h> // For controlling servo motors on ESP32 microcontrollers

//defining the PIN
#define DHT_PIN 15
#define LDR1 35 
#define LDR2 34 
#define MOTOR 18 

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

float minAngle=30.0;
float controlFac=0.75;

float select_medicine = 0;

float minAngle_A =28;
float controlFac_A =0.55;

float minAngle_B =26;
float controlFac_B =0.65;

float minAngle_C =25;
float controlFac_C =0.6;

Servo motor;

//Wifi and mqtt clients
WiFiClient espClient;
PubSubClient mqttClient(espClient); 
DHTesp dhtSensor;

char tempAr[6];  // Array to store temperature data for MQTT transmission
char lightAr[6]; // Array to store light data for MQTT transmission
char humidAr[6]; // Array to store humidity data for MQTT transmission

void setupWifi();
void calculateAngle(double lightintensity, double D);

void setup() {
  Serial.begin(115200);
  setupWifi();
  setupMqtt();
  dhtSensor.setup(DHT_PIN, DHTesp::DHT22); 

  pinMode(LDR1, INPUT);
  pinMode(LDR2, INPUT);

  motor.attach(MOTOR, 500, 2400);
}

// main loop function
void loop() {
  if(!mqttClient.connected()){ 
    connectToBroker(); 
  }
  mqttClient.loop(); 

  updateHumidity();
  Serial.println("Humidity Value - " + String(humidAr));
  mqttClient.publish("HUMIDITY-VAL",humidAr); 

  updateTemperature();
  Serial.println("Temperature Value - " + String(tempAr));
  mqttClient.publish("TEMPERATURE-VAL",tempAr); 

  updateLightIntensity(); 
  mqttClient.publish("LIGHT-INTENSITY",lightAr);

  delay(1000);
  
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//Configuring MQTT server connection 

void setupMqtt(){
  // Change to your computer's IP address as seen from the Wokwi simulator
  mqttClient.setServer("10.10.0.1", 1883);  // Use your computer's local IP address here
  mqttClient.setCallback(receiveCallback);
}

void setupWifi(){
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println("Wokwi-GUEST");
  WiFi.begin("Wokwi-GUEST", "");

  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  //After connection
  Serial.println("WiFi connected");
  Serial.println("IP adress:");
  Serial.println(WiFi.localIP());
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
//setting message reception callback

void receiveCallback(char* topic, byte* payload, unsigned int length){ 
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  char payloadCharAr[length+1]; // Add +1 for null terminator
  for (int i = 0; i < length; i++){
    Serial.print((char)payload[i]);
    payloadCharAr[i] = (char)payload[i];
  }
  payloadCharAr[length] = '\0'; // Add null terminator

  Serial.println();

  if (strcmp(topic,"OFFSET-ANG")==0){
      minAngle = atof(payloadCharAr);
      Serial.println(minAngle);
  }
  if (strcmp(topic,"CONTROL-FAC")==0){
      controlFac = atof(payloadCharAr);
      Serial.println(controlFac);
  }
  if (strcmp(topic,"DROP-DOWN")==0){
      select_medicine = atof(payloadCharAr);
      Serial.println(select_medicine);
  }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
// Function to establish connection with MQTT broker and subscribe to relevant topics

void connectToBroker(){ 
  while(!mqttClient.connected()){
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    
    // Try to connect
    if(mqttClient.connect(clientId.c_str())){
      Serial.println("connected");
      mqttClient.subscribe("OFFSET-ANG");
      mqttClient.subscribe("CONTROL-FAC");
      mqttClient.subscribe("DROP-DOWN");  
    } else{
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
}

void updateTemperature(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity(); 
  String(data.temperature, 2).toCharArray(tempAr, 6);
}

void updateHumidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity(); 
  String(data.humidity, 2).toCharArray(humidAr, 6);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
 // Calculate light intensity by LDRS

void updateLightIntensity() {
  float sensorRight = analogRead(LDR1); 
  float sensorLeft = analogRead(LDR2);
  Serial.println("Right LDR Value - " + String(sensorRight));
  Serial.println("Left LDR Value - " + String(sensorLeft));

  if(sensorRight > sensorLeft) { 
    mqttClient.publish("MAXIMUM-INT-LDR","RIGHT LDR SHOWS MAXIMUM INTENSITY"); 
    double D = 0.5;
    float intensity = (sensorRight)/(4063); 
    Serial.println("LDR_Right : "+String(sensorRight)+"  "+String(intensity));
    String(intensity, 2).toCharArray(lightAr, 6);
    calculateAngle(intensity,D);
   
  }else if(sensorRight < sensorLeft){
    mqttClient.publish("MAXIMUM-INT-LDR","LEFT LDR SHOWS MAXIMUM INTENSITY"); 
    double D = 1.5;
    float intensity = (sensorLeft)/(4063-32);
    Serial.println("LDR_Left : "+String(sensorLeft)+"  "+String(intensity));
    String(intensity, 2).toCharArray(lightAr, 6);
    calculateAngle(intensity,D);

  }else{
    mqttClient.publish("MAXIMUM-INT-LDR","BOTH SHOWS SAME INTENSITY"); 
    double D = 0; 
    float intensity = (sensorLeft)/(4063-32);
    Serial.println("LDR_Left = LDR_Right : "+String(sensorLeft)+"  "+String(intensity));
    String(intensity, 2).toCharArray(lightAr, 6);
    calculateAngle(intensity,D);  
  }
}

void calculateAngle(double lightintensity,double D){
  if(select_medicine == 1){ 
    double angle = minAngle_A *D +(180.0-minAngle_A)*lightintensity*controlFac_A ;
    Serial.println("Servo motor angle is : " + String(angle));
    motor.write(angle);
  }
  else if(select_medicine == 2){
    double angle = minAngle_B *D +(180.0-minAngle_B)*lightintensity*controlFac_B;
    Serial.println("Servo motor angle is : " + String(angle));
    motor.write(angle);
  }
  else if(select_medicine == 3){  
    double angle = minAngle_C *D +(180.0-minAngle_C)*lightintensity*controlFac_C;
    Serial.println("Servo motor angle is : " + String(angle));
    motor.write(angle); 
  }
  else{
    double angle = minAngle *D +(180.0-minAngle)*lightintensity*controlFac; 
    Serial.println("Servo motor angle is : " + String(angle));
    motor.write(angle);
  }
} 