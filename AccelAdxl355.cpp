
#include "AccelAdxl355.h"

#include "CircularBuffer.h"
#include "AveragingBuffer.h"

#include <SPI.h>

#include "Arduino.h"

#include <stdint.h>



// For debug only:
extern int gotReadings;
extern int intCount;



extern AveragingBuffer avgs;



static READING_CALLBACK_T cb_global = NULL; // Not a good way of doing this...


#define FIFO_END_INDICATOR 0x000002
#define FIFO_X_INDICATOR   0x000001


// Down-shifts by 4, keeping sign.
static int32_t toInt32(uint32_t p)
{
  if (p & 0x800000)
  {
    return (int32_t) ((p >> 4) | 0xFFF80000);
  }
  else
  {
    return (int32_t) (p >> 4);
  }
}




extern void sample(void);
extern void trigger(void);


void receiveTripleByte(uint32_t v3)
{
  static int pos = 0;  // 0 = X, 1 = Y, 2 = Z
  static int32_t v33 [5];

  if ((v3 & FIFO_X_INDICATOR) == FIFO_X_INDICATOR)
  {
    // An X value has been received.
    v33[0] = toInt32(v3);
    // Discard any previously received Y,Z values
    v33[1] = 0;
    v33[2] = 0;

    pos = 1; // next value will be a Y
  }
  else
  {
    if (pos == 0)
    {
      // We were expecting a X value. Discard and keep waiting
    }
    else if (pos == 1)
    {
      // Y value
      v33[1] = toInt32(v3);
      pos ++;
    }
    else
    {
      // Z value
      v33[2] = toInt32(v3);
      pos = 0;
      float readings[3];
      readings[0] = 16384./256000.*(float)v33[0];
      readings[1] = 16384./256000.*(float)v33[1];
      readings[2] = 16384./256000.*(float)v33[2];
      if (cb_global != NULL)
      {
          cb_global(readings);
      }
    }
  }
}




///////////////////////////////////////////////////////////////////////////////
/////////// SPI communications  ///////////////////////////////////////////////


// The CS pin to the accelerometer
const int CsPin_ = 0;

// SPI.beginTransaction() must already have been called
uint8_t doSpiRead(uint8_t addr)
{
  digitalWrite(CsPin_, LOW);
  uint16_t x = SPI.transfer16(  (((uint16_t)addr) << 9) | 0x01FF );
  digitalWrite(CsPin_, HIGH);
  delay(2);
  return (uint8_t) (x & 0x00FF);
}

// SPI.beginTransaction() must already have been called
void doSpiWrite(uint8_t addr, uint8_t data)
{
  digitalWrite(CsPin_, LOW);
  uint16_t x = SPI.transfer16(  (((uint16_t)addr) << 9) | 0x0000 | ((uint16_t) data) );
  digitalWrite(CsPin_, HIGH);
  delay(2);
  return;
}

typedef bool (* MULTI_READ_CALLBACK_FN) (uint8_t);

/* For now, use a class to allow us to use function pointers.   */
class multiReading
{
  public:
    static void doSpiMultiRead(uint8_t addr, int num, MULTI_READ_CALLBACK_FN   cb);
};

void multiReading::doSpiMultiRead(uint8_t addr, int num, MULTI_READ_CALLBACK_FN   cb)
{
  digitalWrite(CsPin_, LOW);
  uint8_t x;
  x = SPI.transfer(   (addr << 1) | 0x01   );
  (void) x;  // discard
  for (  ; num > 0; --num)
  {
    x = SPI.transfer(0xFF);

    if (cb)
    {
      if (cb(x))
      {
        // have finished if returns true
        num = 0;
      }
    }
  }

  digitalWrite(CsPin_, HIGH);
  delay(2);
  return;
}

void doSpiBegin(void)
{
  pinMode(CsPin_, OUTPUT);
  digitalWrite(CsPin_, HIGH);
  //  delay(100);

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  SPI.begin();
}

void doSpiEnd(void)
{
  SPI.end();
  SPI.endTransaction();
}





void ADXL355::setFreq(int f)
{
  int i;
  for (i = 0; i < sizeof(clockValues) / sizeof(clockValues[0]); i ++)
  {
    if (f == clockValues[i].freqHz)
    {
      freqOffset = i;
      return;
    }
  }
  //  .. not found. Set invalid value
  freqOffset = -1;

}

void ADXL355::init(void)
{
  doSpiBegin();

  doSpiWrite(0x2D, 0x02);  // Wake up accelerometer. No temperature reading
  {
    uint8_t reg28val = 0x00;  // No high-pass.
    if (freqOffset >= 0 && freqOffset < sizeof(clockValues) / sizeof(clockValues[0]))
    {
      reg28val |= clockValues[freqOffset].adxl355_ODR_LPF;
    }
    else
    {
      // No valid value
      reg28val |= clockValues[0].adxl355_ODR_LPF;
    }
    doSpiWrite(0x28, reg28val);
  }
  doSpiWrite(0x2C, 0x81);  // 2g range

  doSpiEnd();
}


float ADXL355::getFrequency(void)
{
  if (freqOffset >= 0 && freqOffset < sizeof(clockValues) / sizeof(clockValues[0]))
  {
    return (float) clockValues[freqOffset].freqHz;
  }
  else
  {
    return (float) clockValues[0].freqHz;
  }

}

int ADXL355::getTimerReload(void)
{
  if (freqOffset >= 0 && freqOffset < sizeof(clockValues) / sizeof(clockValues[0]))
  {
    return (float) clockValues[freqOffset].timer4reload;
  }
  else
  {
    return (float) clockValues[0].timer4reload;
  }

}

void ADXL355::enterStandby(void)
{
  doSpiBegin();

  doSpiWrite(0x2D, 0x03);

  doSpiEnd();
}

void ADXL355::exitStandby(void)
{
  doSpiBegin();

  doSpiWrite(0x2D, 0x02);

  doSpiEnd();
}



// Units: degC
float ADXL355::readTemperature(void)
{
  doSpiBegin();

  doSpiWrite(0x2D, 0x00);  // Turn on temperature reading
  delay (100);

  uint8_t x1 = doSpiRead(6);
  uint8_t x2 = doSpiRead(7);

  doSpiWrite(0x2D, 0x02);  // Turn off temperature reading

  doSpiEnd();

  return  25.0     +      (    ((float) (((((uint16_t) x1) << 8) + x2) - 1852)) / -9.05    );
}





// Following two functions relate to ADXL355..



// Return true if finished
bool receiveSingleByte(uint8_t u1)
{
  static int pos = 0;
  static uint32_t u3;

  if (pos == 0)
  {
    u3 = 0;
    u3 |= (((uint32_t) u1) << 16);
    pos ++;
  }
  else if (pos == 1)
  {
    u3 |= (((uint32_t) u1) <<  8);
    pos ++;
  }
  else
  {
    u3 |= (((uint32_t) u1) <<  0);
    pos = 0;
    if ((u3 & FIFO_END_INDICATOR) == FIFO_END_INDICATOR)
    {
      return true;
    }
    else
    {
      receiveTripleByte(u3);
      //Serial.print("Data = 0x");
      //Serial.println(u3, HEX);
    }
  }
  return false;
}


// Read out all the data in the FIFO.
void ADXL355::zapFifo(READING_CALLBACK_T cb)
{
  cb_global = cb;  // Assign the global callback while we are doing this function.

  doSpiBegin();

  uint8_t fifo_entries = doSpiRead(0x05);

  //Serial.print("Number of FIFO entries = ");
  //Serial.println(fifo_entries, DEC);

  multiReading D;

  // Read 3 bytes per entry, plus a final 3 bytes for the end-of-buffer
  //  sample point. The reading stops on receiving end-of-buffer, so we
  //  don't actually need to read the number of entries -- could just put
  //  in a really big number here.
  D.doSpiMultiRead(0x11, fifo_entries * 3 + 3, /*callback:  */ receiveSingleByte);

  doSpiEnd();

#ifndef USE_EIC_INTERRUPT
  // There's no EIC interrupt, so need to take account of readings here.

  gotReadings += fifo_entries;
  intCount ++;

#endif  // USE_EIC_INTERRUPT

    cb_global = NULL; // disable the global callback

}

bool ADXL355::getSingleReading(float xyz[3])
{
    return false;  // Not supported
}
