#include <ESP32Encoder.h>
#include "Preferences.h"

//instance of the encode object
ESP32Encoder _encoder;
//long to keep track of microseconds;
unsigned long _previousTime = millis();
unsigned long _loopInterval = 100;
unsigned long _pulsePerRev = 1200;
unsigned long _rpmFilterDepth = 5;

//Used to store settings in non-volatile flash on the ESP32.
Preferences prefs;

#define ENCODER_PWR 16

#define PREFS_NAMESPACE "AngleReader"
#define PREFS_LOOP_INTERVAL "LoopInterval"
#define PREFS_PULSE_PER_REV "PulsePerRev"
#define PREFS_RPM_FILTER_DEPTH "RpmFilterDepth"

void setup(){
	
  //Usual serial setup.  Use 115200, because we need
  //this to be FAST.
	Serial.begin(115200);
	// Enable the weak pull down resistors
	//ESP32Encoder::useInternalWeakPullResistors=DOWN;
	// Enable the weak pull up resistors
	ESP32Encoder::useInternalWeakPullResistors=UP;

	// use pin 36 and 37 for the encoder on the S2 mini.
	_encoder.attachHalfQuad(36, 37);
	
	// set starting count value after attaching
	_encoder.setCount(0);

  //Short delay to allow the serial port to sort itself out.
  delay(3000);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(ENCODER_PWR, OUTPUT);


  //First thing, is to grab the preferences out of flash.
  prefs.begin("AngleReader");

  long loopInterval = prefs.getLong(PREFS_LOOP_INTERVAL);
  if(loopInterval != 0)
    _loopInterval = loopInterval;

  long pulsePerRev = prefs.getLong(PREFS_PULSE_PER_REV);
  if(pulsePerRev != 0)
    _pulsePerRev = pulsePerRev;

  long rpmFilterDepth = prefs.getLong(PREFS_RPM_FILTER_DEPTH);
  if(rpmFilterDepth != 0)
    _rpmFilterDepth = rpmFilterDepth;

  Serial.println();
  Serial.println("--------Loading Settings from flash ---------");
  Serial.print("Loop Interval: ");
  Serial.println(_loopInterval);
  Serial.print("Pulse Per Rev (Half Quadrature): ");
  Serial.println(_pulsePerRev);
  Serial.print("RPM Filter Depth: ");
  Serial.println(_rpmFilterDepth);
  Serial.println("----------------------------------------------");
  Serial.println();
  prefs.end();


  digitalWrite(ENCODER_PWR, HIGH);
  
  //Flash the LED, basically just to tell me that we have got to this point 
  //in the setup.
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(1000);  
  Serial.println("v0.2");
  Serial.println("LoftSoft AngleReader Ready.");
}

void ResetEncoder(long val)
{
  _encoder.setCount(val);
}

void FlashLED()
{
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED on (HIGH is the voltage level)
  delay(200);                       // wait for 200 mil
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED off by making the voltage LOW
  delay(200);  
}

//Main Loop - free running, but interesting stuff is limited
//by time slicing - check my #define at the top to see the
//the timing rate in ms.
void loop(){

  unsigned long currentTime = millis();

  //Limit this loop cycle to the time interval specified.
  if (currentTime - _previousTime > _loopInterval) 
  {
    //Ok, we are greater than the time interval.  Save the
    //current time to compare next time around.
    _previousTime = currentTime;

    static double ang = 0;
    static long pos = 0;
    static double rpm = 0;

    //Check incoming Serial commands before we do anything
    //else.
    if (Serial.available() > 0) 
    {

      char commandChar;
      String parameter;
      // we have data!  Lets see what it is..
      //Get the full string up until the null terminator
      String incomingString = Serial.readString();
      //echo it back for debug
      Serial.println(incomingString);
      //if its not zero length, then we can get the
      //command characeter...
      if(incomingString.length() > 0)
      {
        FlashLED();
        commandChar = incomingString[0];

        //Lets see if there is anything else...
        if(incomingString.length() > 1)
        {
          //there is!  It must have parameters sent with the command.
           parameter = incomingString.substring(1);
        }
      }
      //Now we have a command code, and a potential parameter with it,
      //lets use it!

      //Is it a Reset?
      if(commandChar == 'R')
      {
          Serial.println("Received Reset Command");
          //Yes it is! Reset the encoder count.
          ResetEncoder(0);
      }
      else if(commandChar == 'F')
      {
        prefs.begin(PREFS_NAMESPACE);
        //This is an RPM filter depth command;
        //This should have a parameter with it, to say what the filter
        //depth is..
        if(parameter.length() > 0)
        {
          //cracking - now lets see if we can parse it to a numeric value..
          int val = parameter.toInt();
          //if that succeeded, then val should not be NAN.
          if(val != NAN)
          {
            //...aaaand set it to the filter depth variable.
            _rpmFilterDepth = parameter.toInt();
            //and to flash
            prefs.begin(PREFS_NAMESPACE);
            prefs.putLong(PREFS_RPM_FILTER_DEPTH, _rpmFilterDepth);
            prefs.end();
            Serial.print("Received Filter Command: ");
            Serial.println(_rpmFilterDepth);
          }
        }
      }
      else if(commandChar == 'P')
      {
        prefs.begin(PREFS_NAMESPACE);
        //This is a Pulse Per Rev setting command;
        //This should have a parameter with it,
        if(parameter.length() > 0)
        {
          //good - now lets see if we can parse it to a numeric value..
          int val = parameter.toInt();
          //if that succeeded, then val should not be NAN.
          if(val != NAN)
          {
            //...aaaand set it to the PPR variable
            _pulsePerRev = parameter.toInt();
            //and to flash
            prefs.begin(PREFS_NAMESPACE);
            prefs.putLong(PREFS_PULSE_PER_REV, _pulsePerRev);
            prefs.end(); 
            Serial.print("Received PPR Command: ");
            Serial.println(_pulsePerRev);
          }
        }
      }
      else if(commandChar == 'L')
      {
        prefs.begin(PREFS_NAMESPACE);
        //This is a Loop Interval setting command;
        //This should have a parameter with it,
        if(parameter.length() > 0)
        {
          //good - now lets see if we can parse it to a numeric value..
          int val = parameter.toInt();
          //if that succeeded, then val should not be NAN.
          if(val != NAN)
          {
            //...aaaand set it to the PPR variable
            _loopInterval = parameter.toInt();
            //and to flash
            prefs.begin(PREFS_NAMESPACE);
            prefs.putLong(PREFS_LOOP_INTERVAL, _loopInterval);
            prefs.end(); 
            Serial.print("Received Loop Interval Command: ");
            Serial.println(_loopInterval);
          }
        }
      }
    }

    //Right, now to the meat and potatoes.
    //Get the current count from the encoder library.
    long newPos = _encoder.getCount();
    //Convert this to an angle.  We know that on our encoder we are seeing
    //1200 edges (qudrature).  Make this better in the future, as hard
    //coding this value is not cool.
    double newAng = (((double)newPos) * 0.3);

    //Check to see if the last position is different from the new one..
    if (pos != newPos) 
    {
      //It is!  Fab, the thing is still spinning.

      //Now we can calcualte the RPM from the number of pulses that have happened
      //since the last loop, and the usual rpm formula...
      long newRpm = ((newPos - pos / _loopInterval * 60*10*10) / _pulsePerRev);

      //...and run that through the inlince filter at the filter depth requested.
      rpm = ((rpm * _rpmFilterDepth) + newRpm) / (_rpmFilterDepth + 1);
    
      //oh, and just lift the LED pin high, so that it glows when the 
      //encoder spins.
      digitalWrite(LED_BUILTIN, HIGH); 

      //Write the values out..
      Serial.print(newAng);
      Serial.print(" ");
      Serial.print(newPos);
      Serial.print(" ");
      Serial.println(rpm);
      pos = newPos;
    }
    else 
    {
      //If we dont have a new position on this loop, then 
      //drop the LED pin low again to turn it off.
      digitalWrite(LED_BUILTIN, LOW); 
    }
  }
}

