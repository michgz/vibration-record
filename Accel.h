// Generic ("virtual") accelerometer API


#ifndef ACCEL_H__
#define ACCEL_H__


#include "Reading.h"

class Accel
{
  public:
    virtual void  init(void);
    virtual float readTemperature(void);
    virtual void  setFreq(int);// in Hz
    virtual void  enterStandby(void);
    virtual void  exitStandby(void);
    virtual const char * getName(void);
    virtual const float  scaleFactor(void);
    virtual float  getFrequency(void);
    virtual int    getTimerReload(void);
    //void   zapFifo(void);

virtual void zapFifo(READING_CALLBACK_T cb);
    
    virtual void   softwareReset(void);

    // Store into a linear FIFO as an intermediate to the circular. Should only
    // be needed if the hardware FIFO is not used (i.e. LSM6!)
    virtual bool   useLinearFifo(void);

    virtual bool   getSingleReading(float xyz[3]);

};


#endif  // ACCEL_H__

