
#include <stdbool.h>
#include <stdint.h>

#include "Arduino.h"  // for String

extern void initialiseRtc(void);
extern void displayRtc(bool use_serial_local);
extern uint8_t RtcGetHours(void);
extern void RtcBegin(void);
extern void parseDateTimeToRtc(String m);
