

#include "AccelLsm6.h"

#include <SPI.h>
#include <Wire.h>
#include "Arduino.h"

#include <stdint.h>


// Should always be defined.
#define USE_LINEAR_FIFO


const static bool use_serial = false;

#define LSM6DS_ADDRESS 0x6B


/* 0: use I2C. 1: use SPI.                   *
    Depends on the wiring of the hardware. SPI is recommended, as I2C is slower and this has caused
    issues on occasions with the LSM6 devices.                           */
#define LsmSpi() (1)

const int LsmCsPin_ = 0;

const int LsmIntPin_ = 1;  // PA10


static void LsmWriteReg(const uint8_t addr, const uint8_t data)
{
  if (LsmSpi())
  {
    digitalWrite(LsmCsPin_, LOW);
    delayMicroseconds(5);
    uint16_t x = SPI.transfer16(  (((uint16_t)addr) << 8) | 0x0000 | ((uint16_t) data) );
    delayMicroseconds(1);
    digitalWrite(LsmCsPin_, HIGH);
    delayMicroseconds(30);
  }
  else
  {
    Wire.beginTransmission(LSM6DS_ADDRESS);
    Wire.write(addr);  // start address
    Wire.write(data);
    Wire.endTransmission();
  }
}


static void LsmWriteRegMulti(const uint8_t startAddr, const uint8_t data [], int len)
{
  if (LsmSpi())
  {
    int i = 0;
    while(i < len)
    {
      LsmWriteReg(startAddr + i, data[i]);
      i ++;
    }
  }
  else
  {
    Wire.beginTransmission(LSM6DS_ADDRESS);
    Wire.write(startAddr);  // start address
    int i = 0;
    while(i < 0)
    {
      Wire.write(data[i]);
      i ++;
    }
    Wire.endTransmission();
  }

}

static uint8_t LsmReadByte(const uint8_t addr)
{
  if (LsmSpi())
  {
    digitalWrite(LsmCsPin_, LOW);
    delayMicroseconds(5);
    uint16_t x = SPI.transfer16(  (((uint16_t)addr) << 8) | 0x80FF );
    delayMicroseconds(1);
    digitalWrite(LsmCsPin_, HIGH);
    delayMicroseconds(30);
    return (uint8_t) (x & 0x00FF);
  }
  else
  {
    uint8_t x;

    Wire.beginTransmission(LSM6DS_ADDRESS);
    Wire.write(addr);
    Wire.endTransmission();

    Wire.requestFrom(LSM6DS_ADDRESS, 1);
    x = Wire.read();

    return x;
  }
}

static uint16_t LsmReadShort(const uint8_t addr)
{
  uint16_t x = 0;

  if (LsmSpi())
  {
    digitalWrite(LsmCsPin_, LOW);
    delayMicroseconds(5);
    (void) SPI.transfer(  (((uint8_t)addr) << 0) | 0x80 );
    x += 0x001 * (uint16_t) SPI.transfer(0xFF);
    x += 0x100 * (uint16_t) SPI.transfer(0xFF);
    delayMicroseconds(1);
    digitalWrite(LsmCsPin_, HIGH);
    delayMicroseconds(30);
  }
  else
  {
    Wire.beginTransmission(LSM6DS_ADDRESS);
    Wire.write(addr);  // start address
    Wire.endTransmission();

    Wire.requestFrom(LSM6DS_ADDRESS, 2);

    x += 0x001 * (uint16_t) Wire.read();
    x += 0x100 * (uint16_t) Wire.read();
  }
  return x;

}

static uint32_t LsmReadLong(const uint8_t addr)
{
  uint32_t x = 0;

  if (LsmSpi())
  {
    digitalWrite(LsmCsPin_, LOW);
    delayMicroseconds(5);
    (void) SPI.transfer(  (((uint16_t)addr) << 0) | 0x80 );
    x += 0x00000001 * (uint32_t) SPI.transfer(0xFF);
    x += 0x00000100 * (uint32_t) SPI.transfer(0xFF);
    x += 0x00010000 * (uint32_t) SPI.transfer(0xFF);
    x += 0x01000000 * (uint32_t) SPI.transfer(0xFF);
    delayMicroseconds(1);
    digitalWrite(LsmCsPin_, HIGH);
    delayMicroseconds(50);
  }
  else
  {
    Wire.beginTransmission(LSM6DS_ADDRESS);
    Wire.write(addr);  // start address
    Wire.endTransmission();

    Wire.requestFrom(LSM6DS_ADDRESS, 4);

    x += 0x00000001 * (uint32_t) Wire.read();
    x += 0x00000100 * (uint32_t) Wire.read();
    x += 0x00010000 * (uint32_t) Wire.read();
    x += 0x01000000 * (uint32_t) Wire.read();
  }
  return x;

}

void LSM6DS::init()
{
  if (LsmSpi())
  {
    pinMode(LsmCsPin_, OUTPUT);
    digitalWrite(LsmCsPin_, HIGH);
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
    SPI.begin();
  }
  else
  {
    Wire.begin();
    Wire.setClock(75000);
  }

  uint8_t x;

  //LsmWriteReg(0x12, 0x05);  // Reset the accel

  delay(750);

  x = LsmReadByte(0x0F);

  if (use_serial)
  {
    Serial.print("Got WHO_AM_I: 0x");
    Serial.print(x, HEX);
    Serial.println(" (expected 0x69).");
  }

  LsmWriteReg(0x10, 0x40);  // CTRL_1: 104Hz data rate, 2g scale
  LsmWriteReg(0x11, 0x00);  // CTRL_2: no gyro
  LsmWriteReg(0x12, 0x44);  // CTRL_3: BDU enabled
  //LsmWriteReg(0x19, 0x04);  // CTRL_10: Turn on functions, hopefully to solve the problems with FIFO

  /* Not used: Turn on FIFO; left here to show how to enable if needed    */
  //LsmWriteRegMulti(0x08, (const uint8_t []) {0x01, 0x00, 0x26}, 3);
    // FIFO_CTRL_3: accel to FIFO.
    // FIFO_CTRL_4: no other sources
    // FIFO_CTRL_5: 104Hz FIFO data rate, continuous mode

  /* Turn off FIFO.   */
  LsmWriteReg(0x0A, 0x00);

  /* Note on FIFO: the FIFO was enabled through registers 0x08, 0x09, 0x0A and (in this case) 0x19. Experimentation  *
      showed up difficulties with artifacts and erroneous readings that couldn't be resolved. It's suspected that is
      a fundamental flaw in the hardware. More thorough investigation could be performed, but for now recommended not
      to use.
  */

  LsmWriteReg(0x0D, 0x01);  // INT1_CTRL: XL_DATA_READY.

  if (LsmSpi())
  {
    SPI.end();
  }

}

void LSM6DS::softwareReset(void)
{
  LsmWriteReg(0x12, 0x45);  // reset

  delay(200);
}

float LSM6DS::getFrequency(void)
{
  return 104.;   // only 1 rate implemented
}

void LSM6DS::setFreq(int x)
{
  ;  // No action -- can't change this
}

int LSM6DS::getTimerReload(void)
{
  // this is not particularly critical, since the LSM has an enormous FIFO -- and in fact
  //  we are now not even using the FIFO on that device.
  return 256;   // 256 = 1 per second
}

// Units: degC
float LSM6DS::readTemperature(void)
{
#if 0
  // Accessing the SPI here can interfere with the SD card writing.
  uint16_t x = LsmReadShort(0x20);

  float x_f = (float) x;
  if (x & 0x8000)
  {
    x_f = x_f - 65536.;
  }

  return 25.0   + x_f / 16.0;
#endif
  return -1.0;
}



int32_t U16_TO_I32 (uint16_t v11)
{
  int32_t v22;
  if (v11 & 0x8000)
  {
    v22 = (uint32_t) v11 | 0xFFFF8000;
  }
  else
  {
    v22 = (int32_t) v11;
  }
  return v22;
}


void LSM6DS::zapFifo(READING_CALLBACK_T cb)
{


}

bool LSM6DS::getSingleReading(float xyz[3])
{
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
    digitalWrite(LsmCsPin_, LOW);
    delayMicroseconds(5);
    SPI.begin();

    uint16_t x = 0, y = 0, z = 0;
    const uint16_t addr = 0x28;

    (void) SPI.transfer(  (((uint8_t)addr) << 0) | 0x80 );
    x += 0x001 * (uint16_t) SPI.transfer(0xFF);
    x += 0x100 * (uint16_t) SPI.transfer(0xFF);
    y += 0x001 * (uint16_t) SPI.transfer(0xFF);
    y += 0x100 * (uint16_t) SPI.transfer(0xFF);
    z += 0x001 * (uint16_t) SPI.transfer(0xFF);
    z += 0x100 * (uint16_t) SPI.transfer(0xFF);
    SPI.end();
    delayMicroseconds(5);
    digitalWrite(LsmCsPin_, HIGH);

    xyz[0] = 256.0 * (float)x;
    xyz[1] = 256.0 * (float)y;
    xyz[2] = 256.0 * (float)z;

    return true;
}






