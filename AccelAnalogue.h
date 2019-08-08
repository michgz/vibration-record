#ifndef ACCEL_ANALOGUE_H__
#define ACCEL_ANALOGUE_H__


#include "Accel.h"

class AccelAnalogue: public Accel
{
  private:
    char deviceName [9] = "Analogue";

  public:
    virtual void  init(void);
    virtual float readTemperature(void);
    virtual void  setFreq(int);// in Hz
    virtual void  enterStandby(void) {;}  // no action
    virtual void  exitStandby(void) {;}  // no action
    virtual const char * getName(void) {return deviceName;}
    virtual const float  scaleFactor(void) {return 1./100.;}  // reading-to-g's conversion. TODO: sensible number here
    virtual float  getFrequency(void) {return 125.0;}
    virtual int    getTimerReload(void) {return 30;}   // any value
    //void   zapFifo(void);
    virtual void zapFifo(READING_CALLBACK_T cb);
    
    virtual void   softwareReset(void) {;}  // no action

    virtual bool   useLinearFifo(void) {return false;}
    virtual bool   getSingleReading(float xyz[3]);

};




#endif // ACCEL_ANALOGUE_H__

