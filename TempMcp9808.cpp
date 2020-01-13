

#include "TempMcp9808.h"

#include <SPI.h>
#include <Wire.h>
#include "Arduino.h"

#include <stdint.h>


#define MCP9808_ADDRESS 0x18

float ReadMCP9808(void)
{
  uint16_t amb_temp;
  
  // Wake up digital interface

  Wire.beginTransmission(MCP9808_ADDRESS);
  Wire.write(0x01);
  Wire.write(0);
  Wire.write(0);
  Wire.endTransmission();

  // Delay enough for a couple of readings

  delay(750);

  // Take the reading
  
  Wire.beginTransmission(MCP9808_ADDRESS);
  Wire.write(0x05);
  Wire.endTransmission();

  Wire.requestFrom(MCP9808_ADDRESS, 2);

  amb_temp = 0x100 * Wire.read() + Wire.read();

  // Return the digital interface to sleep
  
  Wire.beginTransmission(MCP9808_ADDRESS);
  Wire.write(0x01);
  Wire.write(1);
  Wire.write(0);
  Wire.endTransmission();

  return  ((float) (amb_temp & 0x0FFF) ) * 0.0625;

}
