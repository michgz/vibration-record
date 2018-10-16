#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_


#include "Reading.h"

#include <stdint.h>


typedef enum
{
  
    // The circular buffer is just being written into, it is not triggered.
    CIRC_BUFFER_STATE_RECORDING = 0,

    // Has been triggered, but the lock area is still being filled
    CIRC_BUFFER_STATE_TRIGGERED,

    // The lock area has been filled, and has not yet been fully read out.
    CIRC_BUFFER_STATE_FULL,

    // The circular buffer has looped round to the start of the lock area.
    // This is undesirable, as it means any new readings will be discarded.
    CIRC_BUFFER_STATE_LOCK    


  
} CIRC_BUFFER_STATE_T;




class CircularBuffer
{
    private:

    int pointer;
    const int size(void) {return 1000;}
    const int halfLockSize(void) {return 250;}
    
    float buff[1000][3];

    int readPointer;


    bool isTriggered;
    int triggerHead, triggerTail;


    CIRC_BUFFER_STATE_T state;


    int triggerHoldOff;
    bool isTriggeredHoldOff;

    bool isFullBuffer;

    // A few private functions
    void simpleTrigger(void);
    void Clear(void);


    public:

    void Init(void);
    void Add(float data[3]);
    void Trigger(void);
    bool IsFullSample(void);

bool Read(float out[3]);
  
};


extern void addToCircBuffer(READING_T);
extern bool haveFullSample;


extern int targetPtr;
extern int startReadPtr;
extern bool getCircBuff(int32_t out[5], int ptr);
extern uint32_t CircBuffSize(void);
extern void clearCircBuffer(void);
extern bool halted;
extern void clearTrigger(void);


#endif  // CIRCULAR_BUFFER_H_

