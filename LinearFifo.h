#ifndef LINEAR_FIFO_H_
#define LINEAR_FIFO_H_

#include "Reading.h"

// Should always be defined.
#define USE_LINEAR_FIFO



#ifdef USE_LINEAR_FIFO


extern void addToLinearFifo(READING_T xyz);
extern void clearLinearFifo(void);
extern void zapLinearFifo(READING_CALLBACK_T cb);


#endif // USE_LINEAR_FIFO


#endif // LINEAR_FIFO_H_

