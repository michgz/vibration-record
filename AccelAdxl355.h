
#include "Accel.h"
#include "Reading.h"

#include <stdint.h>

// Structure for ADXL355 only
typedef struct
{
  int      freqHz;   // or float?
  uint8_t  adxl355_ODR_LPF;  // value to put into ADXL355 register
  uint16_t timer4reload;    // number of timer 4 ticks between reading out FIFO. Faster sample rates must be read out more often

} ADXL_CLOCK_T;

class ADXL355: public Accel
{
  private:
    char deviceName [8] = "ADXL355";

    // Offset into the clockValues table.
    int freqOffset = -1;  // invalid value

    const ADXL_CLOCK_T clockValues [3] =
    {
      {125, 0x5, 30},
      {250, 0x4, 15},
      {500, 0x3,  6}  // and others...
    };

  public:
    virtual void  init(void);
    virtual float readTemperature(void);
    virtual void  setFreq(int);// in Hz
    virtual void  enterStandby(void);
    virtual void  exitStandby(void);
    virtual const char * getName(void) {return deviceName;}
    virtual const float  scaleFactor(void) {return 1./256000.;}  // reading-to-g's conversion. This is only valid for the +-2g range!
    virtual float  getFrequency(void);
    virtual int    getTimerReload(void);
    //void   zapFifo(void);
    virtual void zapFifo(READING_CALLBACK_T cb);
    
    virtual void   softwareReset(void) {;}  // no action

    virtual bool   useLinearFifo(void) {return false;}
    virtual bool   getSingleReading(float xyz[3]);

};




