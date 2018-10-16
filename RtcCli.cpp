#include "RtcCli.h"



#include <Wire.h>

#include <Arduino.h>



///////////////////////////////////////////////////////////////////////////////
/////////// PCF8523 RTC  //////////////////////////////////////////////////////


static uint8_t bcd2bin (uint8_t val) {
  return val - 6 * (val >> 4);
}
#define PCF8523_ADDRESS 0x68

#define MODE_0   (0<<7)       // always use mode 0 - it has lower battery consumption




static void get(void)
{
    char c [80];
  
    Wire.begin();
    Wire.beginTransmission(PCF8523_ADDRESS);
    Wire.write(0x03);
    Wire.endTransmission();
  
    Wire.requestFrom(PCF8523_ADDRESS, 7);
  
    uint8_t vals [7];
  
    vals[0] = bcd2bin(Wire.read() & 0x7F);     // Seconds
    vals[1] = bcd2bin(Wire.read());            // Minutes
    vals[2] = bcd2bin(Wire.read());            // Hours
    vals[3] = bcd2bin(Wire.read());            // Day
    (void) Wire.read();   // discard day-of-week
    vals[4] = bcd2bin(Wire.read());            // Month
    vals[5] = bcd2bin(Wire.read());            // Year
  
    Wire.begin();
    Wire.beginTransmission(PCF8523_ADDRESS);
    Wire.write(0x0E);
    Wire.endTransmission();
  
    Wire.requestFrom(PCF8523_ADDRESS, 1);
  
    vals[6] = Wire.read();
  
    signed int offs;
  
    if (vals[6] <= 63)
    {
      offs = (signed int)vals[6];
    }
    else
    {
      // extend to a negative number
      offs = 63;
      offs = (~offs) | ((signed int)vals[6]);
    }
  
    sprintf(c, "Current Time = %02d/%02d/%04d,%02d:%02d:%02d,%+d", vals[3], vals[4], vals[5] + 2000, vals[2], vals[1], vals[0], offs);
    Serial.println(String(c));

}



static uint8_t bin2bcd(uint8_t x)
{
    uint8_t y = x / 10;
    return (16 * y + (x - 10*y));
}


static void set(String m)
{
    // Parse a string of form "20/02/2012,17:06:33,+34"
    // Load the RTC with its value
    int nok = 0;
    bool hasOffs = false;
      
  
    // Basic checks
    if (!nok && m.length() < 19 || m.length() > 24) nok = 1; 
    if (!nok && m.charAt(2)  != '/') nok = 2;
    if (!nok && m.charAt(5)  != '/') nok = 3;
    if (!nok && m.charAt(13) != ':') nok = 4;
    if (!nok && m.charAt(16) != ':') nok = 5;
  
    int year = m.substring(6, 10).toInt();
    if (!nok && !(year >= 2000 && year <= 2099))
    {
        nok = 6;
    }

    int month;
    int day;
    int hour;
    int minute;
    int second;
  
    if (!nok)
    {
    
        // All fine. Now load in.
  
  

        day = m.substring(0, 2).toInt();
        month = m.substring(3, 5).toInt();
        hour = m.substring(11, 13).toInt();
        minute = m.substring(14, 16).toInt();
        second = m.substring(17, 19).toInt();
    
    }

    int offs;

    if (!nok)
    {
        if (m.length() > 21)
        {
            if (m.charAt(19) == ',')
            {
                char m1 = m.charAt(20);
                if (m1 >= '0' && m1 <= '9')
                {
                    hasOffs = true;
                    offs = m.substring(20).toInt();
                }
                else if (m1 == '+')
                {
                    hasOffs = true;
                    offs = m.substring(21).toInt();
                }
                else if (m1 == '-')
                {
                    hasOffs = true;
                    offs = -m.substring(21).toInt();
                }
            }
        }
    }

    if (!nok)
    {
        // Write in the new value.

        Wire.beginTransmission(PCF8523_ADDRESS);
        Wire.write(0x03); // start address
        Wire.write(bin2bcd(second));  // seconds in BCD
        Wire.write(bin2bcd(minute));  // minutes in BCD
        Wire.write(bin2bcd(hour));  // hours in BCD
        Wire.write(bin2bcd(day));  // day in BCD
        Wire.write(5);     // weekdays -- 5 = Friday. We don't use this register, so think it can be written as anything.
        Wire.write(bin2bcd(month));  // month as BCD
        Wire.write(bin2bcd(year-2000));  // year as BCD (0x00 = 2000, 0x01 = 2001, etc.)
        Wire.endTransmission();

        if (hasOffs)
        {
    
            // Set offset
            
            Wire.beginTransmission(PCF8523_ADDRESS);
            Wire.write(0x0E);
            Wire.write((uint8_t) (0x7F & ((unsigned int)offs)) | MODE_0);
            Wire.endTransmission();

        }
    
        // set to battery switchover mode
        
        Wire.beginTransmission(PCF8523_ADDRESS);
        Wire.write(0x02);
        Wire.write(0x00);
        Wire.endTransmission();

    }

    if (!nok)
    {
        Serial.println("A new time was written to the RTC chip.");
        if (hasOffs)
        {
            Serial.println("Offset was changed.");
        }
    }
    else
    {
        Serial.println("Time was not changed. There was a problem.");
    }




}


int incomingByte = 0;
const int LED_PIN = 13;
String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete





bool DoRtcCLI(void)
{
  // Run a simple command line on the serial port. Commands are:
  //
  //   > get
  //
  // Returns the current RTC value. E.g.
  //      13/08/2018,20:14:55,+4
  //
  //
  //  > set 13/08/2018,20:14:55,+4
  //
  // Sets a new RTC value.
  //
  //
  //  > run
  //
  // Starts the main (logging) program


  int timeCountDown = 120;  // half-seconds

  Serial.begin(9600);
  while (!Serial)
  {
    delay(500);
    timeCountDown --;
    if (timeCountDown == 0)
    {
      return false;
    }
  }



  Serial.println(" >>>>> ENTERING RTC CLI >>>>>>>>>>>");
  Serial.println("  Supported commands: get, set, run");


  while (true)
  {

    if (Serial.available() > 0)
    {
      char inChar = (char) Serial.read();
      inputString += inChar;
      if (inChar == '\n')
      {
        stringComplete = true;
      }
    }
  
    if (stringComplete)
    {
      Serial.println(inputString);
     
      stringComplete = false;
      if (inputString.startsWith("get"))
      {
        get();
      }
      else if (inputString.startsWith("set"))
      {
        set(inputString.substring(4));
      }
      else if (inputString.startsWith("run"))
      {
        Serial.println("Entering logging program..");
        break;
      }

       inputString = "";
    }

    timeCountDown --;
    if (timeCountDown == 0)
    {
      //;  // No action -- the loop time is not exact
    }
  }

  return false;
}


