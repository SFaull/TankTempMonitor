// currently written for arduino but easily adapted for esp-01


#include <SoftwareSerial.h>
#include <stdio.h>
#include <string.h>
#define SENSOR_COUNT 5

SoftwareSerial arduino(10, 11); // RX, TX

float temp[SENSOR_COUNT];

void setup() 
{
  Serial.begin(9600);
  arduino.begin(9600);   // start a software serial to talk to the ESP
}

void loop() {
  // put your main code here, to run repeatedly:
  if (arduino.available() > 0) 
  {
    String input = arduino.readString();
    parseStr(input);

    for(int i=0; i<SENSOR_COUNT; i++)
      Serial.println(temp[i]);
    Serial.println("");
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
    
    return 0;
}

