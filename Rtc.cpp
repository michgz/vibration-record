#include "Rtc.h"

#include <Wire.h>


#include <RTCZero.h>

RTCZero rtc;


///////////////////////////////////////////////////////////////////////////////
/////////// PCF8523 RTC  //////////////////////////////////////////////////////


static uint8_t bcd2bin (uint8_t val) { return val - 6 * (val >> 4); }
#define PCF8523_ADDRESS 0x68

void initialiseRtc(void)
{
  Wire.begin();
  Wire.beginTransmission(PCF8523_ADDRESS);
  Wire.write(0x03);
  Wire.endTransmission();

  Wire.requestFrom(PCF8523_ADDRESS, 7);

  rtc.setSeconds(bcd2bin(Wire.read() & 0x7F));
  rtc.setMinutes(bcd2bin(Wire.read()));
  rtc.setHours(bcd2bin(Wire.read()));
  rtc.setDay(bcd2bin(Wire.read()));
  (void) Wire.read();   // discard day-of-week
  rtc.setMonth(bcd2bin(Wire.read()));
  rtc.setYear(bcd2bin(Wire.read()));
}


void displayRtc(bool use_serial_local)
{
  if (! use_serial_local) return;

  char c [80];

  sprintf(c, "Current Time = %02d/%02d/%04d,%02d:%02d:%02d", rtc.getDay(), rtc.getMonth(), rtc.getYear() + 2000, rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
  Serial.println(String(c));

}

uint8_t RtcGetHours(void)
{
  return rtc.getHours();
}

void RtcBegin(void)
{
  rtc.begin();
}


void parseDateTimeToRtc(String m)
{
  // Parse a string of form "20/02/2012,17:06:33"
  // Load the RTC with its value
  int nok = 0;


  // Basic checks
  if (!nok && m.length() != 19 && m.length() != 20) nok = 1; // Not sure why 20? But that's what it is.
  if (!nok && m.charAt(2)  != '/') nok = 2;
  if (!nok && m.charAt(5)  != '/') nok = 3;
  if (!nok && m.charAt(13) != ':') nok = 4;
  if (!nok && m.charAt(16) != ':') nok = 5;

  int year = m.substring(6, 10).toInt();
  if (!nok && !(year >= 2000 && year <= 2099))
  {
    nok = 6;
  }

  if (!nok)
  {

    // All fine. Now load in.

    rtc.setDay(m.substring(0, 2).toInt());
    rtc.setMonth(m.substring(3, 5).toInt());
    rtc.setYear(year - 2000);
    rtc.setHours(m.substring(11, 13).toInt());
    rtc.setMinutes(m.substring(14, 16).toInt());
    rtc.setSeconds(m.substring(17, 19).toInt());

  }
}
