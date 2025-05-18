#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <ESP32Servo.h>
#include <math.h>

#define Screen_Width 128
#define Screen_Height 64
#define OLED_reset -1
#define screen_address 0x3C
#define Buzzer 14
#define PB_Cancel 13
#define PB_OK 33
#define PB_UP 27
#define PB_Down 32
#define DHTPIN 25
#define AlarmLed 15
#define LDR_PIN 36  // A0 pin
#define SERVO_PIN 4
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

#define MQTT_BROKER "test.mosquitto.org"
#define MQTT_PORT 1883
#define MQTT_CLIENT_ID "ESP32_Medibox"
#define MQTT_TOPIC_TEMP "medibox/temperature"
#define MQTT_TOPIC_HUMIDITY "medibox/humidity"
#define MQTT_TOPIC_ALARMS "medibox/alarms"
#define MQTT_TOPIC_STATUS "medibox/status"
#define MQTT_TOPIC_COMMAND "medibox/command"
#define MQTT_TOPIC_LIGHT "medibox/light"
#define MQTT_TOPIC_MOTOR "medibox/motor"
#define MQTT_TOPIC_CONFIG "medibox/config"

Adafruit_SSD1306 display(Screen_Width,Screen_Height,&Wire,OLED_reset);
DHTesp dhtSensor;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Servo servoMotor;

unsigned long lastMqttReconnectAttempt = 0;
unsigned long lastSensorPublish = 0;
unsigned long lastStatusPublish = 0;
unsigned long lastLightSampleTime = 0;
unsigned long lastLightSendTime = 0;
const long SENSOR_PUBLISH_INTERVAL = 10000;
const long STATUS_PUBLISH_INTERVAL = 30000;

int days = 0;
int hours = 0;
int minitues = 0;
int seconds = 0;
int UTC_OFFSET = 0;
int timezone_hours = 0;
int timezone_minitues = 0;
bool alarm_enabled = true;
int n_alarms = 2;
int alarm_hours[] = {9,9};
int alarm_minitues[] = {30,32};
bool alarm_triggered[] = {false,false};
int current_mode = 0;
int max_modes = 5;
String modes[] = {"1-Set Time Zone","2-Set Alarm 1","3-Set Alarm 2","4 - Delete Alarm","5 - View Alarms"};

String temp_status = "Normal";
String humidity_status = "Normal";
float current_temp = 0.0;
float current_humidity = 0.0;

// Light monitoring variables
int lightSamplingInterval = 5000;     // Default 5 seconds in milliseconds
int lightSendingInterval = 120000;    // Default 2 minutes in milliseconds
float lightReadings[24];              // Array to store light readings (max 2 minutes at 5 second intervals)
int lightReadingIndex = 0;
int lightReadingsCount = 0;
float currentLightIntensity = 0.0;    // Normalized light intensity value (0-1)

// Servo control variables
float minAngle = 30.0;                // Default minimum angle (θoffset)
float controlFactor = 0.75;           // Default controlling factor (γ)
float idealTemp = 30.0;               // Default ideal storage temperature (Tmed)
int currentServoAngle = 30;           // Current servo angle

void setup()
{
  Serial.begin(115200);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); 
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  print_line("Welcome to Medibox",0,0,2);
  delay(2000);
  display.clearDisplay();
  pinMode(Buzzer, OUTPUT);
  pinMode(AlarmLed, OUTPUT);  
  pinMode(PB_OK, INPUT);
  pinMode(PB_Cancel, INPUT);
  pinMode(PB_UP, INPUT);
  pinMode(PB_Down, INPUT);
  pinMode(LDR_PIN, INPUT);
  
  // Initialize servo
  servoMotor.attach(SERVO_PIN);
  servoMotor.write(minAngle);
  
  dhtSensor.setup(DHTPIN,DHTesp::DHT22);
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Connecting to wifi",0,0,2);
  }
  
  display.clearDisplay();
  print_line("Connected to the wifi",0,0,2);
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
  
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  mqttClient.setCallback(mqttCallback);
}

void loop()
{
  if (!mqttClient.connected()) {
    long now = millis();
    if (now - lastMqttReconnectAttempt > 5000) {
      lastMqttReconnectAttempt = now;
      if (reconnectMqtt()) {
        lastMqttReconnectAttempt = 0;
      }
    }
  } else {
    mqttClient.loop();
  }
  
  static unsigned long lastDisplayRefresh = 0;
  unsigned long currentMillis = millis();
  
  // Update time and check alarms
  updateTime();
  
  // Only refresh the time display every 1 second to avoid flickering
  if (currentMillis - lastDisplayRefresh >= 1000) {
    lastDisplayRefresh = currentMillis;
    print_time_now();
    
    // Check for alarms at the same time we update the display
    if(alarm_enabled == true){
      for(int i=0; i<n_alarms; i++){
        if(alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minitues[i]== minitues){
          ring_alarm(i);
          publishAlarms();
        }
      }
    }
    
    // Check temperature and humidity periodically
  check_Temp_and_Humidity();
  }
  
  // Handle light sampling
  if (currentMillis - lastLightSampleTime >= lightSamplingInterval) {
    lastLightSampleTime = currentMillis;
    sampleLightIntensity();
  }
  
  // Handle light data sending
  if (currentMillis - lastLightSendTime >= lightSendingInterval) {
    lastLightSendTime = currentMillis;
    calculateAndSendAverageLightIntensity();
    // After sending light data, update the servo position
    updateServoPosition();
  }
  
  // Regular sensor publishing
  if (currentMillis - lastSensorPublish >= SENSOR_PUBLISH_INTERVAL) {
    lastSensorPublish = currentMillis;
    publishSensorData();
  }
  
  // Status publishing
  if (currentMillis - lastStatusPublish >= STATUS_PUBLISH_INTERVAL) {
    lastStatusPublish = currentMillis;
    publishStatus();
  }
  
  if(digitalRead(PB_OK) == LOW){
    delay(200);
    goToMenu();
  }
}

void sampleLightIntensity() {
  // Read raw value from LDR
  int rawValue = analogRead(LDR_PIN);
  
  // Normalize to a value between 0 and 1
  // ESP32 ADC has 12-bit resolution (0-4095)
  float normalizedValue = (float)rawValue / 4095.0;
  
  // Store the reading in the array
  lightReadings[lightReadingIndex] = normalizedValue;
  lightReadingIndex = (lightReadingIndex + 1) % (lightSendingInterval / lightSamplingInterval);
  
  // Keep track of how many readings we've collected
  if (lightReadingsCount < (lightSendingInterval / lightSamplingInterval)) {
    lightReadingsCount++;
  }
  
  Serial.print("Light sample: ");
  Serial.println(normalizedValue);
}

void calculateAndSendAverageLightIntensity() {
  if (lightReadingsCount == 0) {
    return; // No readings to average
  }
  
  float sum = 0.0;
  for (int i = 0; i < lightReadingsCount; i++) {
    sum += lightReadings[i];
  }
  
  float newLightIntensity = sum / lightReadingsCount;
  
  // Check if light intensity changed significantly before updating display
  bool significant_change = abs(newLightIntensity - currentLightIntensity) > 0.05;
  
  currentLightIntensity = newLightIntensity;
  
  // Publish the average light intensity to MQTT
  String lightStr = String(currentLightIntensity, 3);
  mqttClient.publish(MQTT_TOPIC_LIGHT, lightStr.c_str());
  
  Serial.print("Average light intensity: ");
  Serial.println(lightStr);
  
  // Only update the display if there's a significant change
  if (significant_change) {
    display.clearDisplay();
    print_line("Light: " + lightStr, 0, 0, 2);
    delay(1000);
  }
}

void updateServoPosition() {
  if (current_temp <= 0) {
    return; // Invalid temperature, don't update servo
  }
  
  // Calculate the angle using the provided equation
  // θ = θoffset + (180 - θoffset) × I × γ × ln((ts/tu) × (T/Tmed))
  float tsToTu = (float)lightSamplingInterval / (float)lightSendingInterval;
  float tToTmed = current_temp / idealTemp;
  float logTerm = log(tsToTu * tToTmed);
  
  // Handle negative or zero values in the log
  if (tsToTu * tToTmed <= 0) {
    logTerm = 0;
  }
  
  float angle = minAngle + (180.0 - minAngle) * currentLightIntensity * controlFactor * logTerm;
  
  // Constrain the angle between minimum and 180 degrees
  if (angle < minAngle) {
    angle = minAngle;
  } else if (angle > 180.0) {
    angle = 180.0;
  }
  
  // Only update if the angle has changed significantly (more than 2 degrees)
  int newAngle = (int)angle;
  if (abs(newAngle - currentServoAngle) > 2) {
    currentServoAngle = newAngle;
    servoMotor.write(currentServoAngle);
    
    // Publish the motor angle to MQTT
    String angleStr = String(currentServoAngle);
    mqttClient.publish(MQTT_TOPIC_MOTOR, angleStr.c_str());
    
    Serial.print("Servo angle updated to: ");
    Serial.println(currentServoAngle);
    
    // Display angle update only when it changes significantly
    display.clearDisplay();
    print_line("Angle: " + angleStr, 0, 0, 2);
    print_line("Light: " + String(currentLightIntensity, 2), 0, 25, 2);
    delay(1000);
  }
}

bool reconnectMqtt() {
  if (mqttClient.connect(MQTT_CLIENT_ID)) {
    Serial.println("Connected to MQTT broker");
    mqttClient.subscribe(MQTT_TOPIC_CONFIG);
    publishStatus();
    publishConfigValues();
    return true;
  } else {
    Serial.print("Failed to connect to MQTT broker, rc=");
    Serial.println(mqttClient.state());
    return false;
  }
}

void publishConfigValues() {
  DynamicJsonDocument doc(1024);
  
  doc["lightSamplingInterval"] = lightSamplingInterval / 1000; // Convert to seconds
  doc["lightSendingInterval"] = lightSendingInterval / 1000;   // Convert to seconds
  doc["minAngle"] = minAngle;
  doc["controlFactor"] = controlFactor;
  doc["idealTemp"] = idealTemp;
  doc["source"] = "esp32"; // Add source to prevent feedback loop
  
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  
  mqttClient.publish(MQTT_TOPIC_CONFIG, buffer, n);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  char message[length + 1];
  for (unsigned int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';
  Serial.println(message);
  
  if (String(topic) == MQTT_TOPIC_CONFIG) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }
    
    bool configChanged = false;
    
    // Update configuration parameters if they exist in the message
    if (doc.containsKey("lightSamplingInterval")) {
      int newInterval = doc["lightSamplingInterval"];
      if (newInterval >= 1 && newInterval <= 60 && newInterval * 1000 != lightSamplingInterval) {
        lightSamplingInterval = newInterval * 1000; // Convert from seconds to milliseconds
        // Reset sampling variables when interval changes
        lightReadingIndex = 0;
        lightReadingsCount = 0;
        lastLightSampleTime = millis();
        configChanged = true;
      }
    }
    
    if (doc.containsKey("lightSendingInterval")) {
      int newInterval = doc["lightSendingInterval"];
      if (newInterval >= 10 && newInterval <= 600 && newInterval * 1000 != lightSendingInterval) {
        lightSendingInterval = newInterval * 1000; // Convert from seconds to milliseconds
        // Reset sampling variables when interval changes
        lightReadingIndex = 0;
        lightReadingsCount = 0;
        lastLightSendTime = millis();
        configChanged = true;
      }
    }
    
    if (doc.containsKey("minAngle")) {
      float newAngle = doc["minAngle"];
      if (newAngle >= 0 && newAngle <= 120 && newAngle != minAngle) {
        minAngle = newAngle;
        updateServoPosition(); // Update servo position with new parameters
        configChanged = true;
      }
    }
    
    if (doc.containsKey("controlFactor")) {
      float newFactor = doc["controlFactor"];
      if (newFactor >= 0 && newFactor <= 1 && newFactor != controlFactor) {
        controlFactor = newFactor;
        updateServoPosition(); // Update servo position with new parameters
        configChanged = true;
      }
    }
    
    if (doc.containsKey("idealTemp")) {
      float newTemp = doc["idealTemp"];
      if (newTemp >= 10 && newTemp <= 40 && newTemp != idealTemp) {
        idealTemp = newTemp;
        updateServoPosition(); // Update servo position with new parameters
        configChanged = true;
      }
    }
    
    // Only publish updated configuration if we actually changed something
    // and avoid the feedback loop
    if (configChanged) {
      // If the message came from Node-RED, don't republish
      if (!doc.containsKey("source") || doc["source"] != "node-red") {
        // Publish updated configuration without triggering another callback
        publishConfigValues();
      }
      
      display.clearDisplay();
      print_line("Config updated", 0, 0, 2);
      delay(1500);
    }
  }
}

void publishSensorData() {
  String tempStr = String(current_temp, 1);
  mqttClient.publish(MQTT_TOPIC_TEMP, tempStr.c_str());
  
  String humStr = String(current_humidity, 1);
  mqttClient.publish(MQTT_TOPIC_HUMIDITY, humStr.c_str());
}

void publishAlarms() {
  DynamicJsonDocument doc(1024);
  JsonArray alarmArray = doc.to<JsonArray>();
  
  for (int i = 0; i < n_alarms; i++) {
    JsonObject alarmObj = alarmArray.createNestedObject();
    alarmObj["hours"] = alarm_hours[i];
    alarmObj["minutes"] = alarm_minitues[i];
    alarmObj["active"] = !alarm_triggered[i] && alarm_enabled && 
                         !(alarm_hours[i] == 0 && alarm_minitues[i] == 0);
  }
  
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  
  mqttClient.publish(MQTT_TOPIC_ALARMS, buffer, n);
}

void publishStatus() {
  DynamicJsonDocument doc(1024);
  doc["connected"] = true;
  doc["time"] = String(hours) + ":" + (minitues < 10 ? "0" : "") + String(minitues);
  doc["temp_status"] = temp_status;
  doc["humidity_status"] = humidity_status;
  doc["light_intensity"] = currentLightIntensity;
  doc["motor_angle"] = currentServoAngle;
  
  char buffer[512];
  size_t n = serializeJson(doc, buffer);
  
  mqttClient.publish(MQTT_TOPIC_STATUS, buffer, n);
}

void print_line(String text , int column,int row,int text_size){
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(column,row);
  display.println(text);
  display.display();
}

void print_time_now(void){
  display.clearDisplay();
  print_line(String(days),0,0,2);
  print_line(":",20,0,2);
  print_line(String(hours),30,0,2);
  print_line(":",50,0,2);
  print_line(String(minitues),60,0,2);
  print_line(":",80,0,2);
  print_line(String(seconds),90,0,2);
}

void updateTime(){
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char timeHour[3];
  strftime(timeHour,3,"%H",&timeinfo);
  hours = atoi(timeHour);

  char timeMinitue[3];
  strftime(timeMinitue,3,"%M",&timeinfo);
  minitues = atoi(timeMinitue);

  char timeSecond[3];
  strftime(timeSecond,3,"%S",&timeinfo);
  seconds = atoi(timeSecond);

  char timeDay[3];
  strftime(timeDay,3,"%d",&timeinfo);
  days = atoi(timeDay);
  }

  void ring_alarm(int i){
    display.clearDisplay();
    print_line("Medicine Time",0,0,2);
    print_line("Alarm " + String(i+1),0,20,2);
    
    String hourStr = String(alarm_hours[i]);
    if(alarm_hours[i] < 10) hourStr = "0" + hourStr;
    String minStr = String(alarm_minitues[i]);
    if(alarm_minitues[i] < 10) minStr = "0" + minStr;
    print_line(hourStr + ":" + minStr,0,40,2);
    
    bool snooze_pressed = false;
    
    while(digitalRead(PB_Cancel) == HIGH && digitalRead(PB_OK) == HIGH){     
      tone(Buzzer,256);
      tone(Buzzer,290);
      digitalWrite(AlarmLed, HIGH);
      delay(200);
    }
    
    noTone(Buzzer);
    digitalWrite(AlarmLed, LOW);
    
    if(digitalRead(PB_OK) == LOW) {
      snooze_alarm(i);
    } else {
      alarm_triggered[i] = true;
      display.clearDisplay();
      print_line("Alarm",0,0,2);
      print_line("Canceled",0,20,2);
      delay(1000);
    }
  }

void snooze_alarm(int alarm_index) {
  int new_minutes = minitues + 5;
  int new_hours = hours;
  
  if(new_minutes >= 60) {
    new_minutes = new_minutes % 60;
    new_hours += 1;
    if(new_hours >= 24) {
      new_hours = 0;
    }
  }
  
  alarm_hours[alarm_index] = new_hours;
  alarm_minitues[alarm_index] = new_minutes;
  alarm_triggered[alarm_index] = false;
  
  display.clearDisplay();
  print_line("Alarm",0,0,2);
  print_line("Snoozed",0,20,2);
  print_line("5 minutes",0,40,2);
  delay(1500);
  
  publishAlarms();
}

int waitForButtonPress(){
  while(true){
    mqttClient.loop();
 
    if(digitalRead(PB_UP) == LOW){
      delay(200);
      return PB_UP;
    }
    else if(digitalRead(PB_Down) == LOW){
      delay(200);
      return PB_Down;
    }
    else if(digitalRead(PB_OK) == LOW){
      delay(200);
      return PB_OK;
    }
    else if(digitalRead(PB_Cancel) == LOW){
      delay(200);
      return PB_Cancel;
    }
    updateTime();
  }
}

  void goToMenu(){
    while(digitalRead(PB_Cancel) == HIGH){
        display.clearDisplay();
        print_line(modes[current_mode],0,0,2);
        delay(200);
    int pressed = waitForButtonPress();
        if(pressed == PB_UP){
          delay(200);
          current_mode +=1;
          current_mode = current_mode % max_modes;
        }
        else if(pressed == PB_Down){
          delay(200);
          current_mode -=1;
          if(current_mode < 0){
            current_mode = max_modes - 1;
          }
        }
        else if(pressed == PB_OK){
          delay(200);       
          run_mode(current_mode);
        }
        else if(pressed == PB_Cancel){
          delay(200);
          break;
        }
    }
  }

void set_time_zone(){
    int temp_hours = timezone_hours;
    while(true){
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hours),0,0,2);
    int pressed = waitForButtonPress();
        if(pressed == PB_UP){
          delay(200);
          temp_hours +=1;
          if(temp_hours > 14){
            temp_hours = -12;
          }
        }
        else if(pressed == PB_Down){
          delay(200);
          temp_hours -=1;
          if(temp_hours < -12){
            temp_hours = 14;
          }
        }
        else if(pressed == PB_OK){
          delay(200);
          timezone_hours = temp_hours;
          break;
        }
        else if(pressed == PB_Cancel){
          delay(200);
          break;
        }
        }
    
    int temp_minitues = timezone_minitues;
    while(true){
    display.clearDisplay();
    print_line("Enter minite: " + String(temp_minitues),0,0,2);
    int pressed = waitForButtonPress();
        if(pressed == PB_UP){
          delay(200);
          temp_minitues +=15;
          temp_minitues = temp_minitues % 60;
        }
        else if(pressed == PB_Down){
          delay(200);
          temp_minitues -=15;
          if(temp_minitues < 0){
            temp_minitues = 45;
          }
        }
        else if(pressed == PB_OK){
          delay(200);
          timezone_minitues = temp_minitues;
          break;
        }
        else if(pressed == PB_Cancel){
          delay(200);
          break;
        }
        }
        display.clearDisplay();        
        UTC_OFFSET = 3600 * timezone_hours + 60* timezone_minitues;
        configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
        print_line("Time Zone is set",0,0,2);
        delay(500);

  publishStatus();
}

void set_alarm(int alarm){
  int temp_hours = alarm_hours[alarm];
    while(true){
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hours),0,0,2);
    int pressed = waitForButtonPress();
        if(pressed == PB_UP){
          delay(200);
          temp_hours +=1;
          temp_hours = temp_hours % 24;
        }
        else if(pressed == PB_Down){
          delay(200);
          temp_hours -=1;
          if(temp_hours < 0){
            temp_hours = 23;
          }
        }
        else if(pressed == PB_OK){
          delay(200);
         alarm_hours[alarm] = temp_hours;
          break;
        }
        else if(pressed == PB_Cancel){
          delay(200);
          break;
        }
        }
    
  int temp_minitues = alarm_minitues[alarm];
    while(true){
    display.clearDisplay();
    print_line("Enter minite: " + String(temp_minitues),0,0,2);
    int pressed = waitForButtonPress();
        if(pressed == PB_UP){
          delay(200);
          temp_minitues +=1;
          temp_minitues = temp_minitues % 60;
        }
        else if(pressed == PB_Down){
          delay(200);
          temp_minitues -=1;
          if(temp_minitues < 0){
            temp_minitues = 59;
          }
        }
        else if(pressed == PB_OK){
          delay(200);
          alarm_minitues[alarm] = temp_minitues;
          break;
        }
        else if(pressed == PB_Cancel){
          delay(200);
          break;
        }
        }
        display.clearDisplay();
        alarm_enabled = true;
  alarm_triggered[alarm] = false;
        print_line("Alarm"+ String(alarm+1) +"is set",0,0,2);
        delay(500);

  publishAlarms();
}

void run_mode(int mode){
  if(mode == 0){
   set_time_zone();
  }
  else if(mode == 1){
    set_alarm(0);
  }
  else if(mode == 2){
    set_alarm(1);
  }
  else if(mode == 3){
    delete_specific_alarm();
  }
  else if(mode == 4){
    view_alarms();
  }
}

void check_Temp_and_Humidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  current_temp = data.temperature;
  current_humidity = data.humidity;
  
  if (data.temperature > 32){
    temp_status = "High";
    display.clearDisplay();
    print_line("Temparature is high",0,20,1);
    delay(500);
  }
  else if (data.temperature < 24){
    temp_status = "Low";
    display.clearDisplay();
    print_line("Temparature is low",0,20,1);
    delay(500);
  }
  else {
    temp_status = "Normal";
  }
  
  if (data.humidity > 80){
    humidity_status = "High";
    display.clearDisplay();
    print_line("Humidity is high",0,20,1);
    delay(500);
  }
  else if (data.humidity < 65){
    humidity_status = "Low";
    display.clearDisplay();
    print_line("Humidity is low",0,20,1);
    delay(500);
  }
  else {
    humidity_status = "Normal";
  }
}

void delete_specific_alarm() {
  int alarm_index = 0;
  while(true) {
    display.clearDisplay();
    print_line("Delete Alarm " + String(alarm_index + 1),0,0,2);
    print_line(String(alarm_hours[alarm_index]) + ":" + String(alarm_minitues[alarm_index]),0,20,2);
    
    int pressed = waitForButtonPress();
    if(pressed == PB_UP) {
      delay(200);
      alarm_index = (alarm_index + 1) % n_alarms;
    }
    else if(pressed == PB_Down) {
      delay(200);
      alarm_index--;
      if(alarm_index < 0) {
        alarm_index = n_alarms - 1;
      }
    }
    else if(pressed == PB_OK) {
      delay(200);
      alarm_hours[alarm_index] = 0;
      alarm_minitues[alarm_index] = 0;
      alarm_triggered[alarm_index] = false;
      display.clearDisplay();
      print_line("Alarm " + String(alarm_index + 1) + " deleted",0,0,2);
      delay(1000);
      
      publishAlarms();
      break;
    }
    else if(pressed == PB_Cancel) {
      delay(200);
      break;
    }
  }
}

void view_alarms() {
  display.clearDisplay();
  print_line("Active Alarms:",0,0,2);
  delay(1000);
  
  bool found_active = false;
  for(int i = 0; i < n_alarms; i++) {
    if(!alarm_triggered[i] && alarm_hours[i] != 0 && alarm_minitues[i] != 0 && alarm_enabled) {
      found_active = true;
      display.clearDisplay();
      print_line("Alarm " + String(i+1) + ":",0,0,2);
      
      String hourStr = String(alarm_hours[i]);
      if(alarm_hours[i] < 10) hourStr = "0" + hourStr;
      
      String minStr = String(alarm_minitues[i]);
      if(alarm_minitues[i] < 10) minStr = "0" + minStr;
      
      print_line(hourStr + ":" + minStr,0,20,2);
      print_line("Active",0,40,2);
      
      delay(2000);
    }
  }
  
  if(!found_active) {
    display.clearDisplay();
    print_line("No active",0,0,2);
    print_line("alarms",0,20,2);
    delay(2000);
  }
}

