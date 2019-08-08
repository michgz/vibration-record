
#include "AccelAnalogue.h"

#include <RTCZero.h>
extern RTCZero rtc;

extern void minuteTimer(void);

static void cbEveryMinute(void)
{
  minuteTimer();
}

void AccelAnalogue::init(void)
{
  // Needs the RTC peripheral, but also need to co-exist with RTCZero library.

  // To do this, we co-opt the alarm.
  
  rtc.setAlarmSeconds(0);
  rtc.attachInterrupt(&cbEveryMinute);
  rtc.enableAlarm(rtc.MATCH_SS);

  // Configure PA2,3,8,9 to function "B" (these are the ADC inputs, and "B" is analogue function.)

  PORT->Group[0].WRCONFIG.reg = 0x11000000UL | (1UL << 2) | (1UL << 3) | (1UL << 8) | (1UL << 9);

  PM->APBCMASK.bit.ADC_ = 1;  

  ADC->CTRLA.reg = 0x01;
  delay(20);
  ADC->CTRLA.reg = 0x02;
  while( !!ADC->STATUS.bit.SYNCBUSY ) ;  // Always a delay after enabling ADC
  ADC->REFCTRL.reg = 0x02;  // Reference = VDDANA
  ADC->AVGCTRL.reg = 0x05; // averaging... more to do on this...
  ADC->SAMPCTRL.reg = 0x3F;
  ADC->CTRLB.reg = 0x0514;
  ADC->INPUTCTRL.reg = 0x00031801UL;
  ADC->EVCTRL.reg = 0x00;
  
  ADC->INTFLAG.bit.RESRDY = 1;
  ADC->INTENSET.bit.RESRDY = 1;
  
  NVIC_SetPriority(TC4_IRQn, 3);
  NVIC_EnableIRQ(ADC_IRQn);

  ADC->SWTRIG.bit.START = 1;   // Start!!
}

float AccelAnalogue::readTemperature(void)
{
  return 99.0;
}

void AccelAnalogue::setFreq(int)
{
  // TODO. For now fixed as 125Hz
}

void AccelAnalogue::zapFifo(READING_CALLBACK_T cb)
{
  
}


static volatile unsigned int sums [3];
static volatile unsigned int counts [3];

static void clearSums(void)
{
  sums[0] = 0; counts[0] = 0;
  sums[1] = 0; counts[1] = 0;
  sums[2] = 0; counts[2] = 0;
}

bool AccelAnalogue::getSingleReading(float xyz[3])
{
  if (counts[0] == 0) counts[0] = 1;
  if (counts[1] == 0) counts[1] = 1;
  if (counts[2] == 0) counts[2] = 1;
  xyz[0] = ((float)sums[0])/((float)counts[0]);
  xyz[1] = ((float)sums[1])/((float)counts[1]);
  xyz[2] = ((float)sums[2])/((float)counts[2]);
  clearSums();
  return true;
}

void ADC_Handler(void)
{

  if (!!ADC->INTFLAG.bit.RESRDY)
  {
    ADC->INTFLAG.bit.RESRDY = 1;
    unsigned int v = ADC->INPUTCTRL.bit.INPUTOFFSET;
    unsigned int u = ADC->RESULT.reg;
    if (v == 0)
    {
      sums[0] += u; counts[0] += 1;
    }
    else if (v == 1)
    {
      // This is the reference input -- do nothing with it
    }
    else if (v == 2)
    {
      sums[1] += u; counts[1] += 1;
    }
    else if (v == 3)
    {
      sums[2] += u; counts[2] += 1;
    }

    
  }
}

