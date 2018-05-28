// Tank temp monitor for Arduino Nano - Sam Faull

#include <SoftwareSerial.h>
#include <SerialCommand.h>

/* ADJUSTABLE PARAMETERS, please alter the values in this section to your preference */
#define lowerLimit      25   // temp (in deg C) at which LEDs go blue
#define upperLimit      35   // temp (in deg C) at which LEDs go red

#define beepQuantity    5       // the number of times the buzzer beep 
#define beepDelay       18000   // 3 mins -> the delay before the alarm sounds
#define ALARM_PERIOD    500     // 500ms
#define PUBLISH_PERIOD  12000  // 2 mins = 2 * 60 * 1000
#define UPDATE_PERIOD   1000     // how often (in milliseconds) the sensors will refresh 
                                //   this is also how long the buzzer will beep for 
                                //   (time on = refreshRate, time off = refreshRate) 

#define SENSOR_COUNT  5 // one ambient, 4 tank 
#define BUFFER_SIZE   10

SoftwareSerial esp = SoftwareSerial(11, 12); // RX, TX

SerialCommand sCmd(esp);     // The demo SerialCommand object

// Global variables
int cnt = beepDelay + (2*beepQuantity) + 1;    // global counter variable
bool alarm_enabled = false;
int alarm_count = 0;
bool Standby = false;

int readArray[SENSOR_COUNT][BUFFER_SIZE] = {};
float temp[SENSOR_COUNT] = {0,0,0,0};

unsigned long runTime         = 0,
              alarmTimer      = 0,
              updateTimer     = 0,
              publishTimer    = 0;

void setup()
{
  Serial.begin(115200);  //Start the serial connection with the computer
  esp.begin(38400);   // start a software serial to talk to the ESP

    // Setup callbacks for SerialCommand commands
  sCmd.addCommand("ON",    cmdOn);          // Turns LED on
  sCmd.addCommand("OFF",   cmdOff);         // Turns LED off
  sCmd.addDefaultHandler(unrecognized);  // Handler for command that isn't matched  (says "What?") 
  
  init_IO();  // set up outputs and inputs
  LED_test(); // run LED test sequence
  setTimer(&updateTimer);
  Serial.println("Ready...");
}    
                   
/////////////////////////////////////main code//////////////////////////////////////////////////////////
void loop()           
{

  sCmd.readSerial();     // We don't do much, just process serial commands


  /* Periodically read the inputs */
  if (timerExpired(updateTimer, UPDATE_PERIOD))
  {
    setTimer(&updateTimer);  // reset timer
    read_temp();
    //serial_debug();
    if (!Standby) // only update the LEDs if not in standby mode
    {
      digitalWrite(13, HIGH);
      set_LED_state();
    }
  }

  /* Periodically check if alarm should be enabled*/
  if (timerExpired(alarmTimer, ALARM_PERIOD)) // check for button press periodically
  {
    setTimer(&alarmTimer);  // reset timer
    if (alarm_enabled)
    {
      alarm_count++;
      if (alarm_count > beepDelay*2)
        alarm();
    }
  }
      
}

void init_IO(void)
{
  // set outputs for LEDs + buzzer
  pinMode(2, OUTPUT);   //LED A Blue
  pinMode(3, OUTPUT);   //LED A Red
  pinMode(4, OUTPUT);   //LED B Blue
  pinMode(5, OUTPUT);   //LED B Red
  pinMode(6, OUTPUT);   //LED C Blue
  pinMode(7, OUTPUT);   //LED C Red
  pinMode(8, OUTPUT);   //LED D Blue
  pinMode(9, OUTPUT);   //LED D Red
  pinMode(10, OUTPUT);  //Buzzer
  pinMode(13, OUTPUT);  //Buzzer
}

void LED_test(void)
{
  // test Blue LEDs
  digitalWrite(2, HIGH);
  digitalWrite(4, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(8, HIGH);
  delay(1000);
  // test Red LEDs
  digitalWrite(2, LOW);
  digitalWrite(4, LOW);
  digitalWrite(6, LOW);
  digitalWrite(8, LOW);
  digitalWrite(3, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(9, HIGH);
  delay(1000);
  // LEDs off
  digitalWrite(3, LOW);
  digitalWrite(5, LOW);
  digitalWrite(7, LOW);
  digitalWrite(9, LOW);
  delay(1000); 
}

void read_temp(void)
{
  static int addr = 0;
  float voltage[SENSOR_COUNT];
  static long movingTotal[SENSOR_COUNT];
    
  // get adc reading from temp sensors  
  for (int i=0; i<SENSOR_COUNT; i++)
  {
    // temp is our moving average, so...
    // subtract the temperature value thats about to be wiped from the array
    movingTotal[i] -= readArray[i][addr];
    // read in the new value
    readArray[i][addr] = analogRead(i);
    // add said new value to the moving total
    movingTotal[i] += readArray[i][addr];
    
    // convert the moving total adc reading to voltage (for 3.3v arduino use 3.3)
    voltage[i] = (movingTotal[i]/BUFFER_SIZE) * 5.0;
    voltage[i] /= 1024.0; 

    // calculate temp from voltage
    temp[i] = (voltage[i] - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset to degrees ((voltage - 500mV) times 100)

    if (i==1)
      temp[i] -= 6.3 ;  // 6 deg offset because this sensor is slightly out

    //Serial.println(temp[i]);
  }

  // increment the array address pointer (circular buffer)
  if (addr >= BUFFER_SIZE-1)
    addr=0;
  else
    addr++;

}

void serial_debug(void)
{  
  // print temp to serial
  for (int i=0; i<SENSOR_COUNT; i++)
    Serial.print(temp[0]); Serial.print(" deg C \t");
}

void set_LED_state(void)
{
///////////////////////////////////conditions for sensor 0 /////////////////////////////////////////
 if (temp[0] <= lowerLimit)
 {
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
 }
 else if (temp[0] > lowerLimit && temp[0] < upperLimit)
 {
  digitalWrite(2, HIGH);
  digitalWrite(3, LOW);
 }
 else if (temp[0] >= upperLimit)
 {
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
 }

///////////////////////////////////conditions for sensor 1 ///////////////////////////////////////// 
 if (temp[1] <= lowerLimit) 
 {
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(10, LOW);
  alarm_enabled = true; 
 }
 else if (temp[1] > lowerLimit && temp[1] < upperLimit)
 {
  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);
  alarm_enabled = false;
 }
 else if (temp[1] >= upperLimit)
 {
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  digitalWrite(10, LOW);
  alarm_enabled = false;
 }

 ///////////////////////////////////conditions for sensor 2 /////////////////////////////////////////
 if (temp[2] <= lowerLimit)
 {
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
 }
 else if (temp[2] > lowerLimit && temp[2] < upperLimit)
 {
  digitalWrite(6, HIGH);
  digitalWrite(7, LOW);
 }
 else if (temp[2] >= upperLimit)
 {
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
 }

 ///////////////////////////////////conditions for sensor 3 /////////////////////////////////////////
 if (temp[3] <= lowerLimit)
 {
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
 }
 else if (temp[3] > lowerLimit && temp[3] < upperLimit)
 {
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);
 }
 else if (temp[3] >= upperLimit)
 {
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
 }
}

void alarm(void)
{
  // Buzzer will turn on and off at the refresh rate when the temp dips bellow 45 degrees
  // It will do this 5 times and then stop
  
  static bool alarm_state = 0;
  static int cycles = 0;
  
  alarm_state != alarm_state;

  if (alarm_state)
    digitalWrite(10, HIGH);
  else
  {
    digitalWrite(10, LOW);
    cycles++;
  }

  if (cycles > 10)
  {
    alarm_enabled = false;
    alarm_count = 0;
  }

}

void leds_off(void)
{
  for (int i=2; i<=9; i++)
    digitalWrite(i, LOW);
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

void cmdOn(void)
{
  Serial.println("LEDs ENABLED");   
  Standby = false;
  publishReadings();
}

void cmdOff(void)
{
  Serial.println("LEDs DISABLED");  
  Standby = true;
  leds_off();
  digitalWrite(13, LOW);
  publishReadings();
}

void publishReadings(void)
{
  Serial.println("Publish temperature");
  // upload to te internet
  String cmd;
  /* send the ID of the sensor followed by the temperature (comma seperated) */
  cmd = String(temp[0]) + "," + String(temp[1]) + "," + String(temp[2]) + "," + String(temp[3]) + "," + String(temp[4]);
  esp.println(cmd);
  Serial.println(cmd);
}

// This gets set as the default handler, and gets called when no other command matches. 
void unrecognized()
{
  Serial.println("What?"); 
  Serial.println("(softserial) What?"); 
}
