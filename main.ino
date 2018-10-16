
/* Define to specify a system based on the LSM6DS3 accelerometer   */
//#define LSM6

/* Define to specify a system based on the ADXL355 accelerometer   */
#define ADXL

/* Define to allow the use of USB serial for debugging. If not attached to USB, it will be  *
   disabled automatically so can just leave defined in all cases.                           */
//#undef USE_SERIAL
#define USE_SERIAL


#if (! defined LSM6) && (! defined ADXL)
  #error "Need to define either LSM6 or ADXL"
#endif

#if (defined LSM6) && (defined ADXL)
  #error "Need to define only one of LSM6 and ADXL"
#endif



#ifdef LSM6

  // For Featherwing adalogger
  const int chipSelect = 10;
  
  // Red LED (supported all platforms)
  const int LED_PIN = 13;
  
  // Suitable for Feather adalogger only (Featherwing has no card detect!)
  const int cardDetect = 7;

  #define SAMPLE_SIZE_IMPL   (500)

  // Use the EIC (GPIO edge trigger) interrupt to know when to take a reading.
  #define USE_EIC_INTERRUPT
  
  #define USE_LINEAR_FIFO

#endif


#ifdef ADXL
  
  // For Feather adalogger
  const int chipSelect = 4;
  
  // Green LED (Feather adalogger only)
  const int LED_PIN = 8;
  
  // Red LED (supported all platforms)
  //const int LED_PIN = 13;
  
  // Suitable for Feather adalogger only (Featherwing has no card detect!)
  const int cardDetect = 7;

  #define SAMPLE_SIZE_IMPL   (1000)

  // Readings are kept in the accelerometer FIFO, read them out at timed intervals. No
  //  need for EIC.
  #undef  USE_EIC_INTERRUPT
  
  #undef  USE_LINEAR_FIFO

#endif

// Define to add debugging to every point in the circular buffer. Uses quite a lot of extra memory.
#undef CIRC_BUFF_DEBUG


#include <SPI.h>
#include <Wire.h>
#include <SD.h>

#include <RTCZero.h>

RTCZero rtc;


#define PRINT_ACCEL_MEASUREMENTS()   (1)

static bool use_serial = false;

// Is there a card detect line? It doesn't exist on the Adalogger featherwing,
//  so can't make use of it there. Must always assume that there is a card.
static const bool cardDetectAvailable(void)
{
  return false;
}


// Two debug values.
static int gotReadings = 0;
static int intCount = 0;


///////////////////////////////////////////////////////////////////////////////
/////////// Miscellaneous SAMD21 functions  ///////////////////////////////////



static int _readResolution = 10;
static int _ADCResolution = 10;
static int _writeResolution = 8;

// Wait for synchronization of registers between the clock domains
static __inline__ void syncADC() __attribute__((always_inline, unused));
static void syncADC() {
  while (ADC->STATUS.bit.SYNCBUSY == 1)
    ;
}

// Similar to analogRead, but allows "special" mux values (other than pins)
uint32_t analogRead_Special(uint32_t mux)
{
  uint32_t valueRead = 0;

  syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = mux;  // Selection for the positive ADC input
  ADC->INPUTCTRL.bit.MUXNEG = 0x18; // Selection for the negative mux -- GND

  // Control A
  /*
     Bit 1 ENABLE: Enable
       0: The ADC is disabled.
       1: The ADC is enabled.
     Due to synchronization, there is a delay from writing CTRLA.ENABLE until the peripheral is enabled/disabled. The
     value written to CTRL.ENABLE will read back immediately and the Synchronization Busy bit in the Status register
     (STATUS.SYNCBUSY) will be set. STATUS.SYNCBUSY will be cleared when the operation is complete.

     Before enabling the ADC, the asynchronous clock source must be selected and enabled, and the ADC reference must be
     configured. The first conversion after the reference is changed must not be used.
  */
  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Clear the Data Ready flag
  ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Store the value
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  valueRead = ADC->RESULT.reg;

  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  syncADC();

  return valueRead;//mapResolution(valueRead, _ADCResolution, _readResolution);
}




float ReadTemperature(void)
{
  // Scaling values correct for Variant B of the SAMD21
  //  This seems to work correctly, even though I believe
  //  it should be the A Variant.
  const float _TEMPERATURE_VOLTS_25C = 0.688;
  const float _TEMPERATURE_VOLTS_PERC = 0.00216;

  // Is temperature sensor enabled?
  int z = SYSCTRL->VREF.bit.TSEN;

  if (!z)
  {
    // Enable the temperature sensor, and allow it to settle
    SYSCTRL->VREF.bit.TSEN = 1;
    delay(200);
  }

  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x18);  // 0x18 = Temperature sensor

  // Restore the temperature sensor -- we are done with it
  SYSCTRL->VREF.bit.TSEN = z;

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Do the scaling to degC
  return 25.000 + (( v - _TEMPERATURE_VOLTS_25C) / _TEMPERATURE_VOLTS_PERC);

}


float ReadVBAT(void)
{
  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x07);  // 0x07 = AIN7

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Account for 1/2 voltage divider
  return 2.0 * v;
}



float ReadVCCIO(void)
{
  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x1B);  // 0x1B = VCCIO/4

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Account for 1/4 scaling
  return 4.0 * v;
}



float ReadVCCCORE(void)
{
  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x1A);  // 0x1A = VCCCORE/4

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Account for 1/4 scaling
  return 4.0 * v;
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




///////////////////////////////////////////////////////////////////////////////
/////////// ADXL355 FIFO reading  /////////////////////////////////////////////


class Accel
{
  public:
    void  init(void);
    float readTemperature(void);
    void  setFreq(int);// in Hz
    void  enterStandby(void);
    void  exitStandby(void);
    const char * getName(void);
    const float  scaleFactor(void);
    float  getFrequency(void);
    int    getTimerReload(void);
    void   zapFifo(void);
    void   softwareReset(void);

};

// Structure for ADXL355 only
typedef struct
{
  int      freqHz;   // or float?
  uint8_t  adxl355_ODR_LPF;  // value to put into ADXL355 register
  uint16_t timer4reload;    // number of timer 4 ticks between reading out FIFO. Faster sample rates must be read out more often

} CLOCK_T;

class ADXL355: public Accel
{
  private:
    char deviceName [8] = "ADXL355";

    // Offset into the clockValues table.
    int freqOffset = -1;  // invalid value

    const CLOCK_T clockValues [3] =
    {
      {125, 0x5, 30},
      {250, 0x4, 15},
      {500, 0x3,  6}  // and others...
    };

  public:
    void  init(void);
    float readTemperature(void);
    void  setFreq(int);// in Hz
    void  enterStandby(void);
    void  exitStandby(void);
    const char * getName(void) {return deviceName;}
    const float  scaleFactor(void) {return 1./256000.;}  // reading-to-g's conversion. This is only valid for the +-2g range!
    float  getFrequency(void);
    int    getTimerReload(void);
    void   zapFifo(void);
    void   softwareReset(void) {;}  // no action

};

class LSM6DS: public Accel
{
  private:
    char deviceName [7] = "LSM6DS";

  public:
    void  init(void);
    float readTemperature(void);
    void  setFreq(int);// in Hz
    void  enterStandby(void) {} // unimplemented
    void  exitStandby(void)  {} // unimplemented
    const char * getName(void) {return deviceName;}
    const float  scaleFactor(void) {return 1./16383.;}  // reading-to-g's conversion. This is only valid for the +-2g range!
    float  getFrequency(void);
    int    getTimerReload(void);
    void   zapFifo(void);
    void   softwareReset(void);

};


/* Define the singleton variable!   */
#if defined LSM6
  LSM6DS   theAccel;
#endif
#if defined ADXL
  ADXL355  theAccel;
#endif


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

// Following two functions relate to ADXL355..


#define FIFO_END_INDICATOR 0x000002
#define FIFO_X_INDICATOR   0x000001

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

// Down-shifts by 4, keeping sign.
int32_t toInt32(uint32_t p)
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
      v33[3] = 0x10000;
      v33[4] = 0x10000;
      addToCircBuffer(v33);
    }
  }
}

static unsigned int accessesWithoutReadings = 0;

// Linear FIFO, to be copied into the circular FIFO. Only used for
// LSM6. Needs to hold at least 1 second of data
#define LINEAR_FIFO_SIZE  (600)
#ifdef USE_LINEAR_FIFO
  static uint16_t linearFifo [3] [LINEAR_FIFO_SIZE];
  static int linearFifoPos = 0;
#endif


#ifdef USE_LINEAR_FIFO

void addToLinearFifo(uint16_t x, uint16_t y, uint16_t z)
{
  if (linearFifoPos >= LINEAR_FIFO_SIZE)
  {
    return;  // FIFO full
  }

  linearFifo [0][linearFifoPos   ] = x;
  linearFifo [1][linearFifoPos   ] = y;
  linearFifo [2][linearFifoPos ++] = z;
}

void clearLinearFifo(void)
{
  linearFifoPos = 0;  
}

#endif  // USE_LINEAR_FIFO


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


void LSM6DS::zapFifo(void)
{

#ifdef USE_LINEAR_FIFO

  // There is no FIFO -- simply pull the data off the linear FIFO
  for (int i = 0; i < linearFifoPos; i ++)
  {
    int32_t v33 [5];
    v33[0] = U16_TO_I32(linearFifo [0][i]);
    v33[1] = U16_TO_I32(linearFifo [1][i]);
    v33[2] = U16_TO_I32(linearFifo [2][i]);
    v33[3] = 0x10000;  // A special value to indicate these two values are unused.
    v33[4] = 0;        // unused
    addToCircBuffer(v33);

    if (use_serial && PRINT_ACCEL_MEASUREMENTS())
    {
      // Print every 20th reading to the serial port
      static int paDiv = 0;
      if (paDiv > 20)
      {
        paDiv = 0;
        Serial.print(v33[0], DEC);
        Serial.print(", ");
        Serial.print(v33[1], DEC);
        Serial.print(", ");
        Serial.print(v33[2], DEC);
        Serial.println(".");
      }
      else
      {
        paDiv ++;
      }
    }
  }

  // The linear FIFO has been cleared. Reset the head pointer
  clearLinearFifo();

#endif  // USE_LINEAR_FIFO

}



// Read out all the data in the FIFO.
void ADXL355::zapFifo(void)
{

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

}



///////////////////////////////////////////////////////////////////////////////
/////////// Low-power timer  //////////////////////////////////////////////////

void Setup_TC4(void)
{
  // Set up TC4 to use Generator 6
  GCLK->CLKCTRL.reg =   GCLK_CLKCTRL_GEN_GCLK6     \
                        | GCLK_CLKCTRL_CLKEN         \
                        | GCLK_CLKCTRL_ID_TC4_TC5;

  // Generator 6 runs off the 32kHz xtal. Runs in standby
  GCLK->GENCTRL.reg =   GCLK_GENCTRL_ID(6)         \
                        | GCLK_GENCTRL_SRC_XOSC32K   \
                        | GCLK_GENCTRL_GENEN         \
                        | GCLK_GENCTRL_DIVSEL        \
                        | GCLK_GENCTRL_RUNSTDBY;

  // Generator 6 is divided by 2^2 (i.e. 16384kHz)
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(6) | GCLK_GENDIV_DIV(2);

  // Enable clock to access TC4
  PM->APBCMASK.bit.TC4_ = 1;

  // Set up TC4 as a 1ms tick (approximate -- actually 1/1024 s)
  TC4->COUNT16.CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;
  TC4->COUNT16.CTRLA.bit.WAVEGEN = 1;
  TC4->COUNT16.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV16_Val;
  TC4->COUNT16.CTRLA.bit.PRESCSYNC = TC_CTRLA_PRESCSYNC_PRESC_Val;
  TC4->COUNT16.CTRLA.bit.RUNSTDBY  = 1;
  TC4->COUNT16.CC[1].reg = 10;
  TC4->COUNT16.INTENSET.bit.MC1 = 0;  // Writing zero has no effect
  TC4->COUNT16.CC[0].reg = theAccel.getTimerReload();
  TC4->COUNT16.INTENSET.bit.MC0 = 1;
  NVIC_EnableIRQ(TC4_IRQn);
  NVIC_SetPriority(TC4_IRQn, 0);
  TC4->COUNT16.CTRLA.bit.ENABLE = 1;
  TC4->COUNT16.CTRLBSET.bit.CMD = TC_CTRLBSET_CMD_RETRIGGER_Val;
}

void TC4_enter_standby(void)
{
  TC4->COUNT16.INTENCLR.bit.MC0 = 1;
  NVIC_DisableIRQ(TC4_IRQn);
  NVIC_ClearPendingIRQ(TC4_IRQn);
  TC4->COUNT16.CTRLA.bit.ENABLE = 0;

  // Disable clock to access TC4
  PM->APBCMASK.bit.TC4_ = 0;
}

void TC4_exit_standby(void)
{
  // Enable clock to access TC4
  PM->APBCMASK.bit.TC4_ = 1;

  TC4->COUNT16.INTENSET.bit.MC0 = 1;
  NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_EnableIRQ(TC4_IRQn);
  TC4->COUNT16.CTRLA.bit.ENABLE = 1;
}

static volatile int gotTimeInterrupt = 0;

// Low-level interrupt handler
void TC4_Handler(void)
{
#if 0
  // Indicate tick frequency on green LED (adafruit logger only)
  if (TC4->COUNT16.INTFLAG.bit.MC1)
  {
    digitalWrite(8, HIGH);
  }
  else if (TC4->COUNT16.INTFLAG.bit.MC0)
  {
    digitalWrite(8, LOW);
  }
#endif

  gotTimeInterrupt = 1;

  TC4->COUNT16.INTFLAG.reg |= 0x3B;  // Clear the flags
  NVIC_ClearPendingIRQ(TC4_IRQn);
}



///////////////////////////////////////////////////////////////////////////////
/////////// Triggering and the Circular Buffer  ///////////////////////////////



// Size in triples
#define CIRC_BUFF_SIZE   (2*SAMPLE_SIZE_IMPL)

// Size in triples. Size of the stored sample. Must be strictly less than CIRC_BUFF_SIZE
#define SAMPLE_SIZE  (SAMPLE_SIZE_IMPL)


#ifdef CIRC_BUFF_DEBUG
  #define CIRC_BUFF_WIDTH  (5)
  static int32_t circBuff[CIRC_BUFF_SIZE][5];
#else
  #define CIRC_BUFF_WIDTH  (3)
  static int32_t circBuff[CIRC_BUFF_SIZE][3];
#endif
static int circBuffPtr = 0;


static int32_t averages[3];
static bool averages_valid = false;

uint32_t calculateDeviation(int32_t in[3])
{
  // Calculate a squared distance from the average. This could be a
  // euclidean distance, here we use maximum absolute difference
  // on any axis.

  uint32_t b = 0;
  int32_t a;

  // Find X-axis deviation
  if (in[0] > averages[0])
  {
    a = in[0] - averages[0];
  }
  else
  {
    a = averages[0] - in[0];
  }
  if (a > b)
  {
    b = a;
  }

  // Find Y-axis deviation
  if (in[1] > averages[1])
  {
    a = in[1] - averages[1];
  }
  else
  {
    a = averages[1] - in[1];
  }
  if (a > b)
  {
    b = a;
  }

  // Find Z-axis deviation
  if (in[2] > averages[2])
  {
    a = in[2] - averages[2];
  }
  else
  {
    a = averages[2] - in[2];
  }
  if (a > b)
  {
    b = a;
  }

  return /*b*b*/b;

}



bool triggered = false;
int targetPtr = 0;
int startReadPtr = 0;
bool haveFullSample = false;
bool halted = false;


// Register a trigger event
void trigger(void)
{

  digitalWrite(LED_PIN, HIGH);

  triggered = true;
  haveFullSample = false;
  targetPtr = circBuffPtr + (SAMPLE_SIZE / 2);
  if (targetPtr >= CIRC_BUFF_SIZE)
  {
    targetPtr -= CIRC_BUFF_SIZE;
  }


  if (targetPtr >= SAMPLE_SIZE)
  {
    startReadPtr = targetPtr - SAMPLE_SIZE;
  }
  else
  {
    startReadPtr = (targetPtr + CIRC_BUFF_SIZE) - SAMPLE_SIZE;
  }


}

// Signal that a full buffer (SAMPLE_SIZE samples) is complete.
void fullSample(void)
{
  haveFullSample = true;
}


// Halt the buffering.
void halt(void)
{
  halted = true;
}


void clearTrigger(void)
{
  // Clear the trigger now it's been dealt with

  digitalWrite(LED_PIN, LOW);

  halted = false;
  triggered = false;
  haveFullSample = false;

}

static uint32_t max_dev = 0;


// Restore the circular buffer to its starting state. Must be
//  done after halting buffering.
void clearCircBuffer(void)
{
  max_dev = 0;
  circBuffPtr = 0;
  averages_valid = false;
}


void addToCircBuffer(int32_t data[CIRC_BUFF_WIDTH])
{
  if (halted)
  {
    return;   // throw away all new data while halted
  }

  if (triggered &&   \
      ((circBuffPtr == startReadPtr) || (circBuffPtr + 1 == startReadPtr) || (circBuffPtr + 1 == CIRC_BUFF_SIZE && startReadPtr == 0)))
  {
    // We're about to overwrite our sample. Halt until further notice
    halt();
    return;
  }

  if (circBuffPtr < CIRC_BUFF_SIZE)
  {

    memcpy( circBuff[circBuffPtr++] , data, sizeof(int32_t) * CIRC_BUFF_WIDTH );

    if (triggered)
    {
      if (circBuffPtr == targetPtr || (circBuffPtr == CIRC_BUFF_SIZE && targetPtr == 0))  // take into account we haven't wrapped around yet.
      {
        fullSample();
      }
    }
    else
    {
      if (averages_valid)
      {
        uint32_t k = calculateDeviation(data);
        if (k > getTrigger())
        {
          trigger();
        }

        if (k > max_dev) max_dev = k;
      }
    }

  }

  if (circBuffPtr >= CIRC_BUFF_SIZE)
  {
    // Loop round
    circBuffPtr = 0;

    // Calculate averages for this last loop
    int32_t sums[3];
    int count;

    sums[0] = 0; sums[1] = 0; sums[2] = 0;

    for (count = 0; count < CIRC_BUFF_SIZE; count ++)
    {
      sums[0] += circBuff[count][0];
      sums[1] += circBuff[count][1];
      sums[2] += circBuff[count][2];

      //      if (count == (CIRC_BUFF_SIZE - 100))
      //      {
      //        Serial.print("Typical X = ");
      //        Serial.println(circBuff[count][0], HEX);
      //        Serial.print("Typical Y = ");
      //        Serial.println(circBuff[count][1], HEX);
      //        Serial.print("Typical Z = ");
      //        Serial.println(circBuff[count][2], HEX);
      //      }

    }

    //    Serial.print("Sum X = ");
    //    Serial.println(sums[0], HEX);
    //    Serial.print("Sum Y = ");
    //    Serial.println(sums[1], HEX);
    //    Serial.print("Sum Z = ");
    //    Serial.println(sums[2], HEX);

    averages[0] = sums[0] / count;
    averages[1] = sums[1] / count;
    averages[2] = sums[2] / count;

    averages_valid = true;

    //    Serial.print("Average X = ");
    //    Serial.println(averages[0], DEC);
    //    Serial.print("Average Y = ");
    //    Serial.println(averages[1], DEC);
    //    Serial.print("Average Z = ");
    //    Serial.println(averages[2], DEC);

    //      Serial.print("Max Deviation = ");
    //      Serial.println(max_dev, DEC);

    max_dev = 0;
  }
}


///////////////////////////////////////////////////////////////////////////////
/////////// Configuration File reading  ///////////////////////////////////////


// The configuration file is named "setup.txt" (note lowercase), and on startup
//  it is read an renamed to "setupold.txt". Therefore it will only be used once.



int sdReadLnN(File f, char * buf, int len)
{
  bool loop = true;
  int result = 0;
  while (loop)
  {
    signed char d = f.read();
    if (d >= 0x10)
    {
      if (len > 0)
      {
        *buf = d;
        buf ++;
        len --;
        result ++;
      }
    }
    else if (result > 0 || d < 0)
    {
      loop = false;
    }
  }
  return result;
}


bool sdReadConfigPair(File f, char * buf1, int len1, char * buf2, int len2)
{
  char c [100];
  int h;

  h = sdReadLnN(f, c, 98);
  if (h)
  {
    char * m = strstr(c, "="); // find the first occurance of an equals
    if (!m)
    {
      return false;
    }
    int i1 = m - c;

    if (i1 > len1)
    {
      i1 = len1;
    }
    strncpy(buf1, c, i1);
    if (i1 < len1)
    {
      buf1[i1] = '\0';
    }
    else
    {
      buf1[len1 - 1] = '\0'; // Ensure null-terminated
    }

    m ++;
    int i2 = strlen(m);
    if (i2 > len2)
    {
      i2 = len2;
    }
    strncpy(buf2, m, i2);
    if (i2 < len2)
    {
      buf2[i2] = '\0';
    }
    else
    {
      buf2[len2 - 1] = '\0'; // Ensure null-terminated
    }

    if (i1 > 0 && i2 > 0)
    {
      return true;
    }

  }

  return false;


}


void sdCopyFile(String nameFrom, String nameTo)
{
  File from, to;
  from = SD.open(nameFrom, FILE_READ);
  if (from)
  {
    if (SD.exists(nameTo))
    {
      SD.remove(nameTo);
    }
    to = SD.open(nameTo, FILE_WRITE);
    if (to)
    {
      signed char Byte = 0;
      while (Byte >= 0)
      {
        Byte = from.read();
        if (Byte >= 0)
        {
          to.write(Byte);
        }
      }
      to.close();
    }
    from.close();
  }
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

static int triggerLevel = 10000;

void setTrigger(int x)
{
  triggerLevel = x;
}

int getTrigger(void)
{
  return triggerLevel;
}




bool writeToLog(int what)
{
  // what = 0:  log an "ON" message.
  // what = 1:  log the contents of the sample buffer
  // what = 2:  log a "HEARTBEAT" message
  // what = 3:  log a software reset due to fault


  // Ensure accel chip select is firmly off.
#ifndef LSM6
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
#endif

  // We must make sure the slow EIC interrupt doesn't interfere. Unfortunately that means we will definitely
  // miss some readings.
  NVIC_DisableIRQ(EIC_IRQn);

  //Serial.println("Triggered");


  SD.begin(chipSelect);

  File dataFile;

  {
    char c [40];
    sprintf(c, "%04d%02d%02d.CSV", 2000 + rtc.getYear(), rtc.getMonth(), rtc.getDay());
    dataFile = SD.open(String(c), FILE_WRITE);
  }

  // if the file is available, write to it:
  if (dataFile) {

    char c [100];

    sprintf(c, "%02d/%02d/%04d,%02d:%02d:%02d,", rtc.getDay(), rtc.getMonth(), rtc.getYear() + 2000, rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    dataFile.println(String(c));

    if (what == 0 || what == 2)
    {
      sprintf(c, "S=%s Tacc=%0.2f Tint=%0.2f Vbat=%0.3f VCCIO=%0.3f VCCCORE=%0.3f", theAccel.getName(), theAccel.readTemperature(), ReadTemperature(), ReadVBAT(), ReadVCCIO(), ReadVCCCORE());
      dataFile.println(String(c));
      if (what == 2)
      {
        dataFile.println("HEARTBEAT");
      }
      else
      {
        dataFile.println("ON");
      }
    }
    else if (what == 3)
    {
      dataFile.println("FAULT");
    }
    else if (what == 1 && haveFullSample)   // doesn't hurt to test this flag again
    {
      sprintf(c, "S=%s C=%d F=%0.2f", theAccel.getName(), getTrigger(), theAccel.getFrequency());
      dataFile.println(String(c));
      int readPtr = startReadPtr;
      while (readPtr != targetPtr)
      {
        if (0) ;
#ifdef CIRC_BUFF_DEBUG
        else if ((circBuff[readPtr][3] & 0xFFFF0000uL) == 0)
        {
          // Include the extra information if it falls inside the 16-bit limits
          sprintf(c, "%d,%d,%d,%04X,%X", circBuff[readPtr][0], circBuff[readPtr][1], circBuff[readPtr][2], circBuff[readPtr][3], circBuff[readPtr][4]);
        }
#endif  // CIRC_BUFF_DEBUG
        else
        {
          // No extra information
          sprintf(c, "%d,%d,%d", circBuff[readPtr][0], circBuff[readPtr][1], circBuff[readPtr][2]);
        }
        dataFile.println(String(c));
        readPtr ++;
        if (readPtr >= CIRC_BUFF_SIZE)
        {
          readPtr = 0;
        }
      }
    }
    dataFile.close();
  }
  else
  {
    //Serial.println("Can't open file for writing");
  }

#ifdef USE_EIC_INTERRUPT
  NVIC_EnableIRQ(EIC_IRQn);
#endif

}


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


///////////////////////////////////////////////////////////////////////
/////// Interrupt-triggered SPI (for LSM6 only)     ////////////  /////





void EIC_Handler (void)
{
  intCount ++;

  if (intCount <= 150)  // expect about 107 readings per second. Place a limit
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

#ifdef USE_LINEAR_FIFO
    addToLinearFifo(x, y, z);
#else
    // Nothing defined here. EIC reading is currently only supported when using the linear FIFO (i.e.
    //  with LSM6).
#endif  // USE_LINEAR_FIFO
    gotReadings ++;
  }
  if (intCount > 3000)
  {
    NVIC_DisableIRQ(EIC_IRQn); // "Panic button". Stop if we get too many of these interrupts
  }

  EIC->INTFLAG.bit.EXTINT10 = 1;  // clear flag
  NVIC_ClearPendingIRQ(EIC_IRQn);

}

// Enable input D1 (SAMD PA10) as an interrupt source.
void enableInterrupt(void)
{
    PM->APBAMASK.bit.EIC_ = 1;
    EIC->CTRL.bit.ENABLE = 1;
    while (EIC->STATUS.bit.SYNCBUSY) ;
    EIC->CONFIG[1].bit.SENSE2 = EIC_CONFIG_SENSE2_HIGH_Val; // I10. 8*1+2 = 10

    PORT->Group[0].PINCFG[10].bit.PMUXEN = 1;
    PORT->Group[0].PMUX[5].bit.PMUXE = PORT_PMUX_PMUXE_A_Val;  // I10. 2*5+ (even) = 10. Mux to A = "EXTINT"
  
    EIC->INTENSET.bit.EXTINT10 = 1;
    //EIC->WAKEUP.bit.WAKEUPEN10 = 1;
    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_EnableIRQ(EIC_IRQn);
}


// Disable input D1 (SAMD PA10) as an interrupt source.
// Not used.
void disableInterrupt(void)
{
    // Not tested!!
    EIC->INTENCLR.bit.EXTINT10 = 1;
    NVIC_DisableIRQ(EIC_IRQn);

    EIC->CTRL.bit.ENABLE = 0;
    PM->APBAMASK.bit.EIC_ = 0;
}



///////////////////////////////////////////////////////////////////////////////
/////////// Main program  /////////////////////////////////////////////////////

void setup() {
  // Open serial communications and wait for port to open:
#ifdef USE_SERIAL
  Serial.begin(9600);
  delay(2000);  // wait for serial port to connect. Needed for native USB port only
  if (!Serial)
  {
    use_serial = false;
  }
  else
  {
    use_serial = true;
  }
#else
  delay(2000);
  use_serial = false;
#endif

  rtc.begin();

  if (use_serial) Serial.print("Initializing SD card...");

  // Set up LED output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // start off

  // Set up card detect
  if (cardDetectAvailable())
  {
    pinMode(cardDetect, INPUT_PULLUP);
  }

  // Ensure accel chip select is firmly off.
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);

  pinMode(9, INPUT);  // D9 = AIN7 (VBAT)

  SD.begin(chipSelect);

  if (use_serial) Serial.println("Begin reading config...");

  if (SD.exists("setup.txt"))
  {

    if (use_serial) Serial.println("Found file");

    File dataFile = SD.open("setup.txt", FILE_READ);
    if (dataFile)
    {

      if (use_serial) Serial.println("Opened setup.txt file for reading");

      char c[20];
      char d[30];
      while (sdReadConfigPair(dataFile, c, 20, d, 30))
      {
        if (strcmp(c, "TRIGGER") == 0)
        {
          setTrigger(String(d).toInt());
        }
        //if (strcmp(c, "DATETIME") == 0)
        //{
          //parseDateTimeToRtc(d);
        //}
        if (strcmp(c, "FREQ") == 0)
        {
          theAccel.setFreq(String(d).toInt());
        }
      }
      dataFile.close();
    }

    //if (use_serial) Serial.print("Copying file ...");

    //sdCopyFile("setup.txt", "setupold.txt");

    //if (use_serial) Serial.println(" done.");

    //    SD.remove("setup.txt");
  }

  if (use_serial) Serial.println("");
  if (use_serial) Serial.print("Device ID Register = ");
  if (use_serial) Serial.println(DSU->DID.reg, HEX);  // Reads 0x10010305.
  // (    SAMD32G18A, 256KB Flash, 32KB RAM, 48 pin package
  //         Revision 3, Die 0, Cortex-M0+).

  initialiseRtc();
  displayRtc(use_serial);

  Setup_TC4();

  if (use_serial)
  {
    Serial.print("Using sampling frequency of : ");
    Serial.print(theAccel.getFrequency(), DEC);
    Serial.println(" Hz");
  }

  theAccel.init();

  if (!cardDetectAvailable() || digitalRead(cardDetect))
  {
    writeToLog(0);
  }

#if defined USE_EIC_INTERRUPT
  NVIC_SetPriority(EIC_IRQn, 3);
  enableInterrupt();
#endif

}


// Don't need to check the heartbeat on every timer tick. Divide it down by a certain
//  fraction -- the exact division is not very important.
static unsigned int heartbeatCheckDivisor = 0;
static int heartbeatLastHour = -1;

void loop() {
  // to run repeatedly:


  if (gotTimeInterrupt)
  {
    // Clear the flag immediately. In theory it should be done in a critical section, but
    //  it should be slow enough that it never matters.
    gotTimeInterrupt = 0;
    heartbeatCheckDivisor ++;

    // Disable the interrupts while we're accessing the linear FIFO
    NVIC_DisableIRQ(EIC_IRQn);

    
    theAccel.zapFifo();


    int x  = gotReadings;
    gotReadings = 0;
    
    
    int y = intCount;
    intCount = 0;
    

    if (use_serial)
    {
      Serial.print ("Readings: ");
      Serial.print (x);
      Serial.print(",  Interrupts: ");
      Serial.println(y);
    }

#ifdef USE_EIC_INTERRUPT
    /*
    if (y == 0 && digitalRead(1))
    {
      // Kick-start
      NVIC_SetPendingIRQ(EIC_IRQn);
    }
    */
    NVIC_EnableIRQ(EIC_IRQn);
#endif  // USE_EIC_INTERRUPT


  }
  __DSB();
  __ISB();

  if (heartbeatCheckDivisor >= 16)
  {
    heartbeatCheckDivisor = 0;
    if (heartbeatLastHour == -1)
    {
      // Don't send a heartbeat just after turning on.
      heartbeatLastHour = rtc.getHours();
    }
    else
    {
      if (heartbeatLastHour != rtc.getHours())
      {
        heartbeatLastHour = rtc.getHours();
        writeToLog(2);
      }
    }

    if (accessesWithoutReadings > 10)
    {
      // Something has gone wrong with the accelerometer. Reset it.

      theAccel.softwareReset();
      theAccel.init();

      clearCircBuffer();
#ifdef USE_LINEAR_FIFO
      clearLinearFifo();
#endif

      writeToLog(3);
    }
  }

  if (haveFullSample)
  {
    // The circular buffer has been filled for us to zap out the data.


    writeToLog(1);

    // Re-start buffering
    if (halted)
    {
      clearCircBuffer();
    }
    clearTrigger();


    // We can ignore any interrupts that have occurred while we've been busy
    gotTimeInterrupt = 0;

  }

  if (!gotTimeInterrupt && !haveFullSample)
  {
    SCB->SCR = ~(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);   // only IDLE mode. We want to be able to wake fast.
    // Don't use SLEEP ON EXIT mode.

    __DSB();
    __ISB();
    __WFI();
  }


}
