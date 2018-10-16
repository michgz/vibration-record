
#include "Accel.h"



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
    //void   zapFifo(void);

void zapFifo(READING_CALLBACK_T cb);
    
    void   softwareReset(void);

    bool   useLinearFifo(void) {return false;}
    bool   getSingleReading(float xyz[3]);

};


extern const int LsmCsPin_;

extern const int LsmIntPin_;


