// currently written for arduino but easily adapted for esp-01
// Libraries needed:
//  Time.h & TimeLib.h:  https://github.com/PaulStoffregen/Time
//  Timezone.h: https://github.com/JChristensen/Timezone
//  SSD1306.h & SSD1306Wire.h:  https://github.com/squix78/esp8266-oled-ssd1306
//  NTPClient.h: https://github.com/arduino-libraries/NTPClient
//  ESP8266WiFi.h & WifiUDP.h: https://github.com/ekstrand/ESP8266wifi
//
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoOTA.h>
#include <WifiUDP.h>
#include <String.h>
#include <NTPClient.h>
#include <Time.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <stdio.h>
#include <string.h>
#include "config.h"

#define SENSOR_COUNT 5
#define ONE_MINUTE   60000  // 60 seconds

// Define NTP properties
#define NTP_OFFSET   0            // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "ca.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)


WiFiClient espClient;
PubSubClient client(espClient);

// Set up the NTP UDP client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, NTP_ADDRESS, NTP_OFFSET, NTP_INTERVAL);

time_t local, utc;

// United Kingdom (London, Belfast)
TimeChangeRule BST = {"BST", Last, Sun, Mar, 1, 60};        // British Summer Time
TimeChangeRule GMT = {"GMT", Last, Sun, Oct, 2, 0};         // Standard Time
Timezone UK(BST, GMT);

const char* deviceName          = "TankTemp";
const char* MQTTtopic           = "TankTemp/";
const char* MQTTtopic_ambient   = "TankTemp/Ambient";
const char* MQTTtopic_sensor1   = "TankTemp/Sensor1";
const char* MQTTtopic_sensor2   = "TankTemp/Sensor2";
const char* MQTTtopic_sensor3   = "TankTemp/Sensor3";
const char* MQTTtopic_sensor4   = "TankTemp/Sensor4";

const char * days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"} ;
const char * months[] = {"Jan", "Feb", "Mar", "Apr", "May", "June", "July", "Aug", "Sep", "Oct", "Nov", "Dec"} ;
const char * ampm[] = {"AM", "PM"} ;

unsigned long runTime         = 0,
              minuteTimer     = 0;

float temp[SENSOR_COUNT];
const char *temp_str[SENSOR_COUNT];

void setup() 
{
  Serial.begin(38400);
  timeClient.begin();   // Start the NTP UDP client
  
  /* Setup WiFi and MQTT */ 
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  wifiManager.autoConnect(deviceName);

  client.setServer(MQTTserver, MQTTport);
  client.setCallback(callback);

  initOTA();

  setTimer(&minuteTimer);
}

void loop() 
{
  /* Check WiFi Connection */
  if (!client.connected()) 
    reconnect();
  client.loop();
  ArduinoOTA.handle();
  
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0) 
  {
    String input = Serial.readString();
    parseStr(input);
    publishReadings();
  }

  if (timerExpired(minuteTimer, ONE_MINUTE))  // get the time every 60 seconds
  {
    setTimer(&minuteTimer);  // reset timer
    getTime();
    
    if (isWakeTime())
      Serial.println("ON");
    else if(isSleepTime())
      Serial.println("OFF");
  }
}

void parseStr(String string)
{
    char charArray[50];
    string.toCharArray(charArray, sizeof(charArray));
    char *pt;
    static unsigned int addr = 0;

    pt = strtok (charArray,",");
    while (pt != NULL) {
        temp[addr] = atof(pt);
        pt = strtok (NULL, ",");
        addr++;
    }
    addr = 0;
}


void getTime(void)
{
  // update the NTP client and get the UNIX UTC timestamp 
  timeClient.update();

  // convert received time stamp to time_t object
  utc = timeClient.getEpochTime();

  local = UK.toLocal(utc);

  /*
  String date;
  String t;
  // now format the Time variables into strings with proper names for month, day etc
  date += days[weekday(local)-1];
  date += ", ";
  date += months[month(local)-1];
  date += " ";
  date += day(local);
  date += ", ";
  date += year(local);

  // format the time to 12-hour format with AM/PM and no seconds
  t += hourFormat12(local);
  t += ":";
  if(minute(local) < 10)  // add a zero if minute is under 10
    t += "0";
  t += minute(local);
  t += " ";
  t += ampm[isPM(local)];

  // Display the date and time
  Serial.println("");
  Serial.print("Local date: ");
  Serial.print(date);
  Serial.println("");
  Serial.print("Local time: ");
  Serial.println(t);
  */
}

bool isWakeTime(void)
{
  bool result = false;
  
  if(hour(local) == 21 && minute(local) == 58)
  {
    result = true;
  }
  
  return result;
}

void getWakeTime(void)
{
  // read from EEPROM
}

void setWakeTime(void)
{
  // save to EEPROM
}

bool isSleepTime(void)
{
  bool result = false;
  
  if(hour(local) == 23 && minute(local) == 0)
  {
    result = true;
  }
  
  return result;
}

void getSleepTime(void)
{
  // read from EEPROM
}

void setSleepTime(void)
{
  // save to EEPROM
}

/* pass this function a pointer to an unsigned long to store the start time for the timer */
void setTimer(unsigned long *startTime)
{
  runTime = millis();    // get time running in ms
  *startTime = runTime;  // store the current time
}

/* call this function and pass it the variable which stores the timer start time and the desired expiry time 
   returns true fi timer has expired */
bool timerExpired(unsigned long startTime, unsigned long expiryTime)
{
  runTime = millis(); // get time running in ms
  if ( (runTime - startTime) >= expiryTime )
    return true;
  else
    return false;
}

void publishReadings(void)
{
  /* publish */

  client.publish(MQTTtopic_sensor1, String(temp[0]).c_str());
  client.publish(MQTTtopic_sensor2, String(temp[1]).c_str());
  client.publish(MQTTtopic_sensor3, String(temp[2]).c_str());
  client.publish(MQTTtopic_sensor4, String(temp[3]).c_str());
  client.publish(MQTTtopic_ambient, String(temp[4]).c_str());

}

void callback(char* topic, byte* payload, unsigned int length) 
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  /* --------------- Print incoming message to serial ------------------ */
  char input[length+1];
  for (int i = 0; i < length; i++) 
    input[i] = (char)payload[i];  // store payload as char array
  input[length] = '\0'; // dont forget to add a termination character

  Serial.print("MQTT message received: "); 
  Serial.println(input);    
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    Serial.print("Attempting MQTT connection... ");
    // Attempt to connect
    if (client.connect(deviceName, MQTTuser, MQTTpassword)) 
    {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      //client.publish("/LampNode/Announcements", "LampNode02 connected");  // potentially not necessary
      // ... and resubscribe
      client.subscribe(MQTTtopic);
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void initOTA(void)
{
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Update Started");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA Update Complete");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}
