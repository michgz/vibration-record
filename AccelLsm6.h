
#ifndef __H_ACCEL_LSM6
#define __H_ACCEL_LSM6


#include "Accel.h"



class LSM6DS: public Accel
{
  private:
    char deviceName [7] = "LSM6DS";

  public:
    virtual void   init(void);
    virtual float  readTemperature(void);
    virtual void   setFreq(int);// in Hz
    virtual void   enterStandby(void) {} // unimplemented
    virtual void   exitStandby(void)  {} // unimplemented
    virtual const char * getName(void) {return deviceName;}
    virtual const float  scaleFactor(void) {return 1./16383.;}  // reading-to-g's conversion. This is only valid for the +-2g range!
    virtual float  getFrequency(void);
    virtual int    getTimerReload(void);
    virtual void   zapFifo(READING_CALLBACK_T cb);
    
    virtual void   softwareReset(void);

    virtual bool   useLinearFifo(void) {return true;}
    virtual bool   getSingleReading(float xyz[3]);

};


extern const int LsmCsPin_;

extern const int LsmIntPin_;

#endif   // __H_ACCEL_LSM6

