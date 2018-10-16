
#include "CircularBuffer.h"


// Include this file to get basic functions (digitalWrite, etc...)
//
#include "Arduino.h"


#include <stdint.h>




#define SAMPLE_SIZE_IMPL 1000
#define LED_PIN  8



int getTrigger1(void) {return 100;}

///////////////////////////////////////////////////////////////////////////////
/////////// Triggering and the Circular Buffer  ///////////////////////////////



// Size in triples
#define CIRC_BUFF_SIZE   (2*SAMPLE_SIZE_IMPL)

// Size in triples. Size of the stored sample. Must be strictly less than CIRC_BUFF_SIZE
#define SAMPLE_SIZE  (SAMPLE_SIZE_IMPL)


        



bool triggered = false;
int targetPtr = 0;
int startReadPtr = 0;
bool haveFullSample = false;
bool halted = false;



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




bool CircularBuffer::IsFullSample(void)
{
    return (state == CIRC_BUFFER_STATE_FULL || state == CIRC_BUFFER_STATE_LOCK);
}


void CircularBuffer::Init(void)
{

    pointer = 0;
    isTriggered = false;
    isTriggeredHoldOff = false;
    isFullBuffer = false;

    state = CIRC_BUFFER_STATE_RECORDING;

    // Begin as held off.
    triggerHoldOff = halfLockSize();
}

void CircularBuffer::Add(float data[3])
{
    if (state == CIRC_BUFFER_STATE_LOCK)
    {
        // Discard all new readings while locked
        return;
    }
  
    if (pointer >= size())
    {
        pointer = 0;

        // Record that we have wrapped around at least once.
        isFullBuffer = true;
    }

    buff[pointer][0] = data[0];
    buff[pointer][1] = data[1];
    buff[pointer][2] = data[2];
    pointer ++;   

    if (pointer >= size())
    {
        pointer = 0;

        isFullBuffer = true;
    }

    if (state == CIRC_BUFFER_STATE_TRIGGERED && pointer == triggerTail)
    {
        // The lock area is now full!!
        state = CIRC_BUFFER_STATE_FULL;
    }
    else if (state == CIRC_BUFFER_STATE_FULL && pointer == triggerHead)
    {
        // We've looped round to the beginning!!
        state = CIRC_BUFFER_STATE_LOCK;
    }

    if (triggerHoldOff > 0)
    {
        triggerHoldOff --;
        if (triggerHoldOff == 0 && isTriggeredHoldOff)
        {
            isTriggeredHoldOff = false;
            if (! isTriggered)
            {
                simpleTrigger();
            }
        }
    }
  
}

bool CircularBuffer::Read(float out[3])
{

    if (state == CIRC_BUFFER_STATE_RECORDING || state == CIRC_BUFFER_STATE_TRIGGERED)
    {
        // Don't have a full sample, so can't yet read out.
        return false;
    }
  
    memcpy(out, buff[readPointer], 3*sizeof(float));
    readPointer ++;
    if (readPointer >= size())
    {
        readPointer = 0;
    }

    if (readPointer == triggerTail)
    {
        // We have reached the end! Untrigger & unlock
        isTriggered = false;
        isTriggeredHoldOff = false;
        if (state == CIRC_BUFFER_STATE_LOCK)
        {
            // If was locked, need to do a full clear
            Clear();
        }
        state = CIRC_BUFFER_STATE_RECORDING;
        triggerHoldOff = halfLockSize();
    }

    return true;
    
}


void CircularBuffer::Clear(void)
{
    state = CIRC_BUFFER_STATE_RECORDING;
    pointer = 0;
    isTriggeredHoldOff = false;
    triggerHoldOff = halfLockSize();
}



void CircularBuffer::Trigger(void)
{
    if (triggerHoldOff > 0)
    {
        // Delay triggering until the hold off is run out.
        isTriggeredHoldOff = true;
    }
    else
    {
        // Can trigger immediately
        if (state == CIRC_BUFFER_STATE_RECORDING)
        {
            simpleTrigger();
        }
    }
}

void CircularBuffer::simpleTrigger(void)
{

    // Indicate triggered
    isTriggered = true;
    state = CIRC_BUFFER_STATE_TRIGGERED;

    // Calculate the last point of the locked buffer.
    triggerTail = pointer + halfLockSize();
    if (triggerTail >= size())
    {
        triggerTail -= size();
    }

    // Calculate the first point of the locked buffer.
    if (pointer >= halfLockSize())
    {
        triggerHead = pointer - halfLockSize();
    }
    else
    {
        if (! isFullBuffer)
        {
            // We're wrapping around, but don't yet have a full buffer's worth of data.
            // Need to cancel the trigger.
            isTriggered = false;
            state = CIRC_BUFFER_STATE_RECORDING;
            return;
        }
        triggerHead = (pointer + size()) - halfLockSize();
    }

    readPointer = triggerHead;
        
}
