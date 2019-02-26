

#include "ProcessorSamd21.h"

#include "Arduino.h"




///////////////////////////////////////////////////////////////////////////////
/////////// Miscellaneous SAMD21 functions  ///////////////////////////////////



static int _readResolution = 10;
static int _ADCResolution = 10;
static int _writeResolution = 8;

// Wait for synchronization of registers between the clock domains
static __inline__ void syncADC() __attribute__((always_inline, unused));
static void syncADC() {
  while (ADC->STATUS.bit.SYNCBUSY == 1)
    ;
}

// Similar to analogRead, but allows "special" mux values (other than pins)
uint32_t analogRead_Special(uint32_t mux)
{
  uint32_t valueRead = 0;

  syncADC();
  ADC->INPUTCTRL.bit.MUXPOS = mux;  // Selection for the positive ADC input
  ADC->INPUTCTRL.bit.MUXNEG = 0x18; // Selection for the negative mux -- GND

  // Control A
  /*
     Bit 1 ENABLE: Enable
       0: The ADC is disabled.
       1: The ADC is enabled.
     Due to synchronization, there is a delay from writing CTRLA.ENABLE until the peripheral is enabled/disabled. The
     value written to CTRL.ENABLE will read back immediately and the Synchronization Busy bit in the Status register
     (STATUS.SYNCBUSY) will be set. STATUS.SYNCBUSY will be cleared when the operation is complete.

     Before enabling the ADC, the asynchronous clock source must be selected and enabled, and the ADC reference must be
     configured. The first conversion after the reference is changed must not be used.
  */
  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x01;             // Enable ADC

  // Start conversion
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Clear the Data Ready flag
  ADC->INTFLAG.reg = ADC_INTFLAG_RESRDY;

  // Start conversion again, since The first conversion after the reference is changed must not be used.
  syncADC();
  ADC->SWTRIG.bit.START = 1;

  // Store the value
  while (ADC->INTFLAG.bit.RESRDY == 0);   // Waiting for conversion to complete
  valueRead = ADC->RESULT.reg;

  syncADC();
  ADC->CTRLA.bit.ENABLE = 0x00;             // Disable ADC
  syncADC();

  return valueRead;//mapResolution(valueRead, _ADCResolution, _readResolution);
}




float ReadTemperature(void)
{
  // Scaling values correct for Variant B of the SAMD21
  //  This seems to work correctly, even though I believe
  //  it should be the A Variant.
  const float _TEMPERATURE_VOLTS_25C = 0.688;
  const float _TEMPERATURE_VOLTS_PERC = 0.00216;

  // Is temperature sensor enabled?
  int z = SYSCTRL->VREF.bit.TSEN;

  if (!z)
  {
    // Enable the temperature sensor, and allow it to settle
    SYSCTRL->VREF.bit.TSEN = 1;
    delay(200);
  }

  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x18);  // 0x18 = Temperature sensor

  // Restore the temperature sensor -- we are done with it
  SYSCTRL->VREF.bit.TSEN = z;

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Do the scaling to degC
  return 25.000 + (( v - _TEMPERATURE_VOLTS_25C) / _TEMPERATURE_VOLTS_PERC);

}


float ReadVBAT(void)
{
  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0xF;  // 1/2x gain

  // Do a "special" read
  int x = analogRead_Special(0x07);  // 0x07 = VBAT/2

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Account for 1/2 voltage divider and 1/2 gain
  return 2.0 * 2.0 * v;
}



float ReadVCCIO(void)
{
  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x1B);  // 0x1B = VCCIO/4

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Account for 1/4 scaling
  return 4.0 * v;
}



float ReadVCCCORE(void)
{
  // Set up the ADC -- using standard "analogRead" infrastructure
  analogReference(AR_INTERNAL1V65);
  analogReadResolution(10);
  ADC->INPUTCTRL.bit.GAIN = 0;  // 1x gain

  // Do a "special" read
  int x = analogRead_Special(0x1A);  // 0x1A = VCCCORE/4

  // Do the scaling to volts
  float v = 1.6500 * (((float) x) / 1023.0);

  // Account for 1/4 scaling
  return 4.0 * v;
}







///////////////////////////////////////////////////////////////////////////////
/////////// Low-power timer  //////////////////////////////////////////////////

void Setup_TC4(int timerReload)
{
  // Set up TC4 to use Generator 6
  GCLK->CLKCTRL.reg =   GCLK_CLKCTRL_GEN_GCLK6     \
                        | GCLK_CLKCTRL_CLKEN         \
                        | GCLK_CLKCTRL_ID_TC4_TC5;

  // Generator 6 runs off the 32kHz xtal. Runs in standby
  GCLK->GENCTRL.reg =   GCLK_GENCTRL_ID(6)         \
                        | GCLK_GENCTRL_SRC_XOSC32K   \
                        | GCLK_GENCTRL_GENEN         \
                        | GCLK_GENCTRL_DIVSEL        \
                        | GCLK_GENCTRL_RUNSTDBY;

  // Generator 6 is divided by 2^2 (i.e. 16384kHz)
  GCLK->GENDIV.reg = GCLK_GENDIV_ID(6) | GCLK_GENDIV_DIV(2);

  // Enable clock to access TC4
  PM->APBCMASK.bit.TC4_ = 1;

  // Set up TC4 as a 1ms tick (approximate -- actually 1/1024 s)
  TC4->COUNT16.CTRLA.bit.MODE = TC_CTRLA_MODE_COUNT16_Val;
  TC4->COUNT16.CTRLA.bit.WAVEGEN = 1;
  TC4->COUNT16.CTRLA.bit.PRESCALER = TC_CTRLA_PRESCALER_DIV16_Val;
  TC4->COUNT16.CTRLA.bit.PRESCSYNC = TC_CTRLA_PRESCSYNC_PRESC_Val;
  TC4->COUNT16.CTRLA.bit.RUNSTDBY  = 1;
  TC4->COUNT16.CC[1].reg = 10;
  TC4->COUNT16.INTENSET.bit.MC1 = 0;  // Writing zero has no effect
  TC4->COUNT16.CC[0].reg = timerReload;
  TC4->COUNT16.INTENSET.bit.MC0 = 1;
  NVIC_EnableIRQ(TC4_IRQn);
  NVIC_SetPriority(TC4_IRQn, 0);
  TC4->COUNT16.CTRLA.bit.ENABLE = 1;
  TC4->COUNT16.CTRLBSET.bit.CMD = TC_CTRLBSET_CMD_RETRIGGER_Val;
}

void TC4_enter_standby(void)
{
  TC4->COUNT16.INTENCLR.bit.MC0 = 1;
  NVIC_DisableIRQ(TC4_IRQn);
  NVIC_ClearPendingIRQ(TC4_IRQn);
  TC4->COUNT16.CTRLA.bit.ENABLE = 0;

  // Disable clock to access TC4
  PM->APBCMASK.bit.TC4_ = 0;
}

void TC4_exit_standby(void)
{
  // Enable clock to access TC4
  PM->APBCMASK.bit.TC4_ = 1;

  TC4->COUNT16.INTENSET.bit.MC0 = 1;
  NVIC_ClearPendingIRQ(TC4_IRQn);
  NVIC_EnableIRQ(TC4_IRQn);
  TC4->COUNT16.CTRLA.bit.ENABLE = 1;
}

volatile int gotTimeInterrupt = 0;

extern int count;

// Low-level interrupt handler
void TC4_Handler(void)
{
#if 0
  // Indicate tick frequency on green LED (adafruit logger only)
  if (TC4->COUNT16.INTFLAG.bit.MC1)
  {
    digitalWrite(8, HIGH);
  }
  else if (TC4->COUNT16.INTFLAG.bit.MC0)
  {
    digitalWrite(8, LOW);
  }
#endif

  gotTimeInterrupt = 1;

  TC4->COUNT16.INTFLAG.reg |= 0x3B;  // Clear the flags
  NVIC_ClearPendingIRQ(TC4_IRQn);


}
