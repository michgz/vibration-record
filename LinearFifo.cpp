#include "LinearFifo.h"

// Linear FIFO, to be copied into the circular FIFO. Only used for
// LSM6. Needs to hold at least 1 second of data
#define LINEAR_FIFO_SIZE  (600)
#ifdef USE_LINEAR_FIFO
  static float linearFifo [3] [LINEAR_FIFO_SIZE];
  static int linearFifoPos = 0;
#endif


#ifdef USE_LINEAR_FIFO

void addToLinearFifo(READING_T xyz)
{
    if (linearFifoPos >= LINEAR_FIFO_SIZE)
    {
        return;  // FIFO full
    }
  
    linearFifo [0][linearFifoPos   ] = xyz[0];
    linearFifo [1][linearFifoPos   ] = xyz[1];
    linearFifo [2][linearFifoPos ++] = xyz[2];
}

void clearLinearFifo(void)
{
    linearFifoPos = 0;  
}

void zapLinearFifo(READING_CALLBACK_T cb)
{

#ifdef USE_LINEAR_FIFO

    // There is no FIFO -- simply pull the data off the linear FIFO
    for (int i = 0; i < linearFifoPos; i ++)
    {
        READING_T v33;
        v33[0] = linearFifo [0][i];
        v33[1] = linearFifo [1][i];
        v33[2] = linearFifo [2][i];
        cb(v33);

#if 0
        if (/*use_serial && PRINT_ACCEL_MEASUREMENTS()*/ false )
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
#endif // 0
    }
  
    // The linear FIFO has been cleared. Reset the head pointer
    clearLinearFifo();
}

#endif  // USE_LINEAR_FIFO



#endif  // USE_LINEAR_FIFO

