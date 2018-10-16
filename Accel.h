// Generic ("virtual") accelerometer API


#ifndef ACCEL_H__
#define ACCEL_H__


#include "Reading.h"

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
    //void   zapFifo(void);

void zapFifo(READING_CALLBACK_T cb);
    
    void   softwareReset(void);

    // Store into a linear FIFO as an intermediate to the circular. Should only
    // be needed if the hardware FIFO is not used (i.e. LSM6!)
    bool   useLinearFifo(void);

    bool   getSingleReading(float xyz[3]);

};


#endif  // ACCEL_H__

