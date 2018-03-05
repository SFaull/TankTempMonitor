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

#define SENSOR_COUNT 50
#define ONE_MINUTE   6000  // 60 
// Define NTP propertiesseconds

#define NTP_OFFSET   0            // In seconds
#define NTP_INTERVAL 60 * 1000    // In miliseconds
#define NTP_ADDRESS  "ca.pool.ntp.org"  // change this to whatever pool is closest (see ntp.org)

#define WAKE_HOUR 17  // 17:00 (5PM)
#define SLEEP_HOUR 23  // 23:00 (11PM)


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
const char* MQTTtopic           = "TankTemp/#";
const char* MQTTtopic_comms     = "TankTemp/Comms";
const char* MQTTtopic_pump      = "TankTemp/Pump";
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

void parseStr(String string)
{
  char charArray[50];
  string.toCharArray(charArray, sizeof(charArray));
  char *pt;
  static unsigned int addr = 0;

  pt = strtok (charArray, ",");
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
}

bool isWakeTime(void)
{
  if (hour(local) == WAKE_HOUR && minute(local) == 0)
    return true;

  return false;
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
  if (hour(local) == SLEEP_HOUR && minute(local) == 0)
    return true;

  return false;
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
  client.publish(MQTTtopic_pump, String(temp[4]).c_str());
  Serial.print("Publish successful");
}

void callback(char* topic, byte* payload, unsigned int length)
{
  //Serial.print("Message arrived [");
  //Serial.print(topic);
  //Serial.print("] ");

  /* --------------- Print incoming message to serial ------------------ */
  char input[length + 1];
  for (int i = 0; i < length; i++)
    input[i] = (char)payload[i];  // store payload as char array
  input[length] = '\0'; // dont forget to add a termination character

  //Serial.print("MQTT message received: ");
  //Serial.println(input);
  
  if (strcmp(topic, MQTTtopic_comms)==0)
  {    
    if(strcmp(input,"ON")==0)
    {
      Serial.print("ON");
    }
    
    if(strcmp(input,"OFF")==0)
    {
      Serial.print("OFF");
    }
  }
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
      client.publish(MQTTtopic_comms, "Connected");  // potentially not necessary
      // ... and resubscribe
      client.subscribe(MQTTtopic_comms);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.print(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup()
{
  //delay(3000); // wait some time for the arduino to boot up
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
    {
      Serial.print("ON");
      client.publish(MQTTtopic_comms, "ON");
    }
    else if (isSleepTime())
    {
      Serial.print("OFF");
      client.publish(MQTTtopic_comms, "OFF");
    }
  }
}



