// Tank temp monitor for Arduino Nano - Sam Faull

/* ADJUSTABLE PARAMETERS, please alter the values in this section to your preference */
#define lowerLimit 25   // temp (in deg C) at which LEDs go blue
#define upperLimit 35   // temp (in deg C) at which LEDs go red

#define beepQuantity 5  // the number of times the buzzer beep 
#define beepDelay 180   // the delay (in seconds) before the alarm sounds
#define refreshRate 100 // how often (in milliseconds) the sensors will refresh 
                        //   this is also how long the buzzer will beep for 
                        //   (time on = refreshRate, time off = refreshRate) 

// Global variables
int cnt = beepDelay + (2*beepQuantity) + 1;    // global counter variable

float tempA,tempB,tempC,tempD = 0;

void setup()
{
  //Serial.begin(9600);  //Start the serial connection with the computer
  init_IO();  // set up outputs and inputs
  LED_test(); // run LED test sequence

}    
                   
/////////////////////////////////////main code//////////////////////////////////////////////////////////
void loop()           
{
  read_temp();
  serial_debug();
  set_LED_state();
  delay(refreshRate);    // waiting for a short period
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
  int i;
  static int addr = 0;
  static int readArrayA[10] = {};
  static int readArrayB[10] = {};
  static int readArrayC[10] = {};
  static int readArrayD[10] = {};
  int readingA;
  int readingB;
  int readingC;
  int readingD;

  // get adc reading from temp sensor  
  readArrayA[addr] = analogRead(0);
  readArrayB[addr] = analogRead(1);
  readArrayC[addr] = analogRead(2);
  readArrayD[addr] = analogRead(3);

  // increment the array address pointer (circular buffer)
  if (addr >= 9)
    addr=0;
  else
    addr++;

  for (i=0; i<=9; i++)
  {
    readingA += readArrayA[i];
    readingB += readArrayB[i];
    readingC += readArrayC[i];
    readingD += readArrayD[i];
  }
  
  readingA = readingA/10;
  readingB = readingB/10;
  readingC = readingC/10;
  readingD = readingD/10;

  // convert adc reading to voltage (for 3.3v arduino use 3.3)
  float voltageA = readingA * 5.0;
  voltageA /= 1024.0; 
  float voltageB = readingB * 5.0;
  voltageB /= 1024.0; 
  float voltageC = readingC * 5.0;
  voltageC /= 1024.0; 
  float voltageD = readingD * 5.0;
  voltageD /= 1024.0; 
    
  // calculate temp from voltage
  tempA = (voltageA - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset to degrees ((voltage - 500mV) times 100)
  tempB = ((voltageB - 0.5) * 100) - 6.3 ;  // 6 deg offset because this sensor is slightly out
  tempC = (voltageC - 0.5) * 100 ;
  tempD = (voltageD - 0.5) * 100 ;
}

void serial_debug(void)
{  
  // print temp to serial
  Serial.print(tempA); Serial.print(" deg C \t");
  Serial.print(tempB); Serial.print(" deg C \t");
  Serial.print(tempC); Serial.print(" deg C \t");
  Serial.print(tempD); Serial.println(" deg C \t");
}

void set_LED_state(void)
{
///////////////////////////////////conditions for sensor A /////////////////////////////////////////
 if (tempA <= lowerLimit)
 {
  digitalWrite(2, LOW);
  digitalWrite(3, LOW);
 }
 else if (tempA > lowerLimit && tempA < upperLimit)
 {
  digitalWrite(2, HIGH);
  digitalWrite(3, LOW);
 }
 else if (tempA >= upperLimit)
 {
  digitalWrite(2, LOW);
  digitalWrite(3, HIGH);
 }

///////////////////////////////////conditions for sensor B ///////////////////////////////////////// 
 if (tempB <= lowerLimit) 
 {
  digitalWrite(4, LOW);
  digitalWrite(5, LOW);
  digitalWrite(10, LOW);
  cnt = beepDelay + (2*beepQuantity) + 1;
 }
 else if (tempB > lowerLimit && tempB < upperLimit)
 {
  alarm();
  digitalWrite(4, HIGH);
  digitalWrite(5, LOW);
 }
 else if (tempB >= upperLimit)
 {
  digitalWrite(4, LOW);
  digitalWrite(5, HIGH);
  digitalWrite(10, LOW);
  cnt = 0;
 }

 ///////////////////////////////////conditions for sensor C /////////////////////////////////////////
 if (tempC <= lowerLimit)
 {
  digitalWrite(6, LOW);
  digitalWrite(7, LOW);
 }
 else if (tempC > lowerLimit && tempC < upperLimit)
 {
  digitalWrite(6, HIGH);
  digitalWrite(7, LOW);
 }
 else if (tempC >= upperLimit)
 {
  digitalWrite(6, LOW);
  digitalWrite(7, HIGH);
 }

 ///////////////////////////////////conditions for sensor D /////////////////////////////////////////
 if (tempD <= lowerLimit)
 {
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
 }
 else if (tempD > lowerLimit && tempD < upperLimit)
 {
  digitalWrite(8, HIGH);
  digitalWrite(9, LOW);
 }
 else if (tempD >= upperLimit)
 {
  digitalWrite(8, LOW);
  digitalWrite(9, HIGH);
 }
}

void alarm(void)
{
// Buzzer will turn on and off at the refresh rate when the temp dips bellow 45 degrees
// It will do this 5 times and then stop

  if (cnt <= (beepDelay + (2*beepQuantity) -1))
  {
    cnt++;
    if (cnt >= beepDelay)
    {
      if (cnt%2)
        digitalWrite(10, HIGH);
      else
        digitalWrite(10, LOW);
    }
  else
      digitalWrite(10, LOW);
  }
}

