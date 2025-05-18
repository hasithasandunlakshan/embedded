#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHTesp.h>
#include <WiFi.h>

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
#define NTP_SERVER     "pool.ntp.org"
#define UTC_OFFSET_DST 0

Adafruit_SSD1306 display(Screen_Width,Screen_Height,&Wire,OLED_reset);
DHTesp dhtSensor;


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
  
  dhtSensor.setup(DHTPIN,DHTesp::DHT22);
  WiFi.begin("Wokwi-GUEST", "", 6);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    display.clearDisplay();
    print_line("Coonnecting to wifi",0,0,2);
  }
  
  display.clearDisplay();
  print_line("Connected to the wifi",0,0,2);
  configTime(UTC_OFFSET, UTC_OFFSET_DST, NTP_SERVER);
}

void loop()
{
  update_time_with_check_alarm();
  check_Temp_and_Humidity();
  if(digitalRead(PB_OK)== LOW ){
    delay(200);
    goToMenu();
    
  }
  
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

//Updates the internal time variables based on the current local time.
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

  void update_time_with_check_alarm(){
  updateTime();
  print_time_now();

  if(alarm_enabled == true){
    for(int i=0; i<n_alarms; i++){
      if(alarm_triggered[i] == false && alarm_hours[i] == hours && alarm_minitues[i]== minitues){
        ring_alarm(i);
      }
    }
  }
  }

  void ring_alarm(int i){
    display.clearDisplay();
    print_line("Medicine Time",0,0,2);
    print_line("Alarm " + String(i+1),0,20,2);
    
    // Format time display
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
    
    // Determine if snooze was pressed
    if(digitalRead(PB_OK) == LOW) {
      snooze_alarm(i);
    } else {
      // Cancel was pressed
      alarm_triggered[i] = true;
      display.clearDisplay();
      print_line("Alarm",0,0,2);
      print_line("Canceled",0,20,2);
      delay(1000);
    }
  }

// Function to snooze alarm for 5 minutes
void snooze_alarm(int alarm_index) {
  // Calculate new alarm time (current time + 5 minutes)
  int new_minutes = minitues + 5;
  int new_hours = hours;
  
  // Handle minute overflow
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
}

int waitForButtonPress(){
  while(true){
 
    if(digitalRead(PB_UP)  == LOW){
      delay(200);
      return PB_UP;
    }
    else if(digitalRead(PB_Down )== LOW){
      delay(200);
      return PB_Down;
    }
    else if(digitalRead(PB_OK )== LOW ){
      delay(200);
      return PB_OK;
    }
    else if(digitalRead(PB_Cancel )== LOW){
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
        int pressed =waitForButtonPress();
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

//set the time zone by getting offset from the user
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

}


void set_alarm(int alarm){
    int temp_hours =  alarm_hours[alarm];
    while(true){
    display.clearDisplay();
    print_line("Enter hour: " + String(temp_hours),0,0,2);
    int pressed =waitForButtonPress();
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
    
    int temp_minitues =  alarm_minitues[alarm];
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
        alarm_triggered[alarm] = false;     //If an alarm is canceled and then set again, the alarm should be set.
        print_line("Alarm"+ String(alarm+1) +"is set",0,0,2);
        delay(500);

}

// run mode that choose from the menu
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

// check temperature and humidity
void check_Temp_and_Humidity(){
  TempAndHumidity data = dhtSensor.getTempAndHumidity();
  if (data.temperature>32){
    display.clearDisplay();
    print_line("Temparature is high",0,20,1);
    delay(500);
  }
  else if (data.temperature<24){
    display.clearDisplay();
    print_line("Temparature is low",0,20,1);
    delay(500);
  }
  if (data.humidity>80){
    display.clearDisplay();
    print_line("Humidity is high",0,20,1);
    delay(500);
  }
  else if (data.humidity<65){
    display.clearDisplay();
    print_line("Humidity is low",0,20,1);
    delay(500);
  }

}

// Function to delete a specific alarm
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
      // Reset the alarm time to indicate it's inactive (midnight)
      alarm_hours[alarm_index] = 0;
      alarm_minitues[alarm_index] = 0;
      alarm_triggered[alarm_index] = false;
      display.clearDisplay();
      print_line("Alarm " + String(alarm_index + 1) + " deleted",0,0,2);
      delay(1000);
      break;
    }
    else if(pressed == PB_Cancel) {
      delay(200);
      break;
    }
  }
}

// Function to view all active alarms
void view_alarms() {
  display.clearDisplay();
  print_line("Active Alarms:",0,0,2);
  delay(1000);
  
  bool found_active = false;
  for(int i = 0; i < n_alarms; i++) {
    // Check if this alarm is active (not triggered and enabled)
    if(!alarm_triggered[i] && alarm_hours[i] != 0 && alarm_minitues[i] != 0 && alarm_enabled) {
      found_active = true;
      display.clearDisplay();
      print_line("Alarm " + String(i+1) + ":",0,0,2);
      
      // Format the time with leading zeros
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