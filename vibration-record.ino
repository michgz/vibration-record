// ============================================================================================
//  Basic Arduino include files
// ============================================================================================


#include <SPI.h>
#include <Wire.h>
#include <SD.h>



// ============================================================================================
//  Arduino library include files (each library must be specifically installed!)
// ============================================================================================

#include <RTCZero.h>




// ============================================================================================
//  Project include files
// ============================================================================================


#include "CircularBuffer.h"
#include "LinearFifo.h"
#include "AveragingBuffer.h"
#include "Rtc.h"
#include "ConfigurationFile.h"
#include "RtcCli.h"
#include "Accel.h"
#include "Board.h"



// Should always be defined.
#define USE_LINEAR_FIFO



extern RTCZero rtc;


/* Define to specify a system based on the LSM6DS3 accelerometer   */
//#define LSM6

/* Define to specify a system based on the ADXL355 accelerometer   */
#define ADXL

/* Define to allow the use of USB serial for debugging. If not attached to USB, it will be  *
   disabled automatically so can just leave defined in all cases.                           */
//#undef USE_SERIAL
#define USE_SERIAL


#if (! defined LSM6) && (! defined ADXL)
  #error "Need to define either LSM6 or ADXL"
#endif

#if (defined LSM6) && (defined ADXL)
  #error "Need to define only one of LSM6 and ADXL"
#endif

// Green LED (Feather adalogger only)
//const int LED_PIN = 8;

// Red LED (supported all platforms)
const int LED_PIN = 13;


#ifdef LSM6

  // For Featherwing adalogger
  const int chipSelect = 10;
  

  
  // Suitable for Feather adalogger only (Featherwing has no card detect!)
  const int cardDetect = 7;

  #define SAMPLE_SIZE_IMPL   (500)

  // Use the EIC (GPIO edge trigger) interrupt to know when to take a reading.
  #define USE_EIC_INTERRUPT
  
  #define USE_LINEAR_FIFO

#endif


#ifdef ADXL
  
  // For Feather adalogger
  const int chipSelect = 4;
  

  
  // Suitable for Feather adalogger only (Featherwing has no card detect!)
  const int cardDetect = 7;

  #define SAMPLE_SIZE_IMPL   (1000)

  // Readings are kept in the accelerometer FIFO, read them out at timed intervals. No
  //  need for EIC.
  #undef  USE_EIC_INTERRUPT
  
  #undef  USE_LINEAR_FIFO

#endif

// Define to add debugging to every point in the circular buffer. Uses quite a lot of extra memory.
#undef CIRC_BUFF_DEBUG








#define PRINT_ACCEL_MEASUREMENTS()   (1)

static bool use_serial = false;

// Is there a card detect line? It doesn't exist on the Adalogger featherwing,
//  so can't make use of it there. Must always assume that there is a card.
static const bool cardDetectAvailable(void)
{
  return false;
}


// Two debug values.
int gotReadings = 0;
int intCount = 0;



///////////////////////////////////////////////////////////////////////////////
/////////// ADXL355 FIFO reading  /////////////////////////////////////////////

#include "Accel.h"

#include "AccelAdxl355.h"
#include "ProcessorSamd21.h"

/* Define the singleton variable!   */
#if defined LSM6
//  LSM6DS   theAccel;
#endif
#if defined ADXL
  ADXL355  theAccelAdxl;
#endif

class Accel   * theAccel;

AveragingBuffer avgs;

CircularBuffer circ;


void addToCircBuffer(READING_T xyz)
{
    circ.Add(xyz);
}

void addToCircBufferNoLinearFifo(READING_T xyz)
{
    circ.Add(xyz);
    avgs.Add(xyz);
    sample();
    if (avgs.IsTriggered())
    {
        trigger();
    }
}

bool configUSE_LINEAR_FIFO(void)
{
    return theAccel->useLinearFifo();
}



static unsigned int accessesWithoutReadings = 0;








static int triggerLevel = 10000;

void setTrigger(int x)
{
  triggerLevel = x;
}

int getTrigger(void)
{
  return triggerLevel;
}




bool writeToLog(int what)
{
  // what = 0:  log an "ON" message.
  // what = 1:  log the contents of the sample buffer
  // what = 2:  log a "HEARTBEAT" message
  // what = 3:  log a software reset due to fault


  // Ensure accel chip select is firmly off.
#ifndef LSM6
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);
#endif

  // We must make sure the slow EIC interrupt doesn't interfere. Unfortunately that means we will definitely
  // miss some readings.
  NVIC_DisableIRQ(EIC_IRQn);

  //Serial.println("Triggered");


  SD.begin(chipSelect);

  File dataFile;

  {
    char c [40];
    sprintf(c, "%04d%02d%02d.CSV", 2000 + rtc.getYear(), rtc.getMonth(), rtc.getDay());
    dataFile = SD.open(String(c), FILE_WRITE);
  }

  // if the file is available, write to it:
  if (dataFile) {

    char c [100];

    sprintf(c, "%02d/%02d/%04d,%02d:%02d:%02d,", rtc.getDay(), rtc.getMonth(), rtc.getYear() + 2000, rtc.getHours(), rtc.getMinutes(), rtc.getSeconds());
    dataFile.println(String(c));

    if (what == 0 || what == 2)
    {
      sprintf(c, "S=%s Tacc=%0.2f Tint=%0.2f Vbat=%0.3f VCCIO=%0.3f VCCCORE=%0.3f", theAccel->getName(), theAccel->readTemperature(), ReadTemperature(), ReadVBAT(), ReadVCCIO(), ReadVCCCORE());
      dataFile.println(String(c));
      if (what == 2)
      {
        dataFile.println("HEARTBEAT");
      }
      else
      {
        dataFile.println("ON");
      }
    }
    else if (what == 3)
    {
      dataFile.println("FAULT");
    }
    else if (what == 1 && circ.IsFullSample())   // doesn't hurt to test this flag again
    {
      sprintf(c, "S=%s C=%d F=%0.2f", theAccel->getName(), getTrigger(), theAccel->getFrequency());
      dataFile.println(String(c));


      float buff [3];
      while (true)
      {
        if (! circ.Read(buff))
        {
            break;
        }
       
        if (0) ;
#ifdef CIRC_BUFF_DEBUG
        else if ((buff[3] & 0xFFFF0000uL) == 0)
        {
          // Include the extra information if it falls inside the 16-bit limits
          sprintf(c, "%0.4f,%0.4f,%0.4f,%04X,%X", buff[0], buff[1], buff[2], buff[3], buff[4]);
        }
#endif  // CIRC_BUFF_DEBUG
        else
        {
          // No extra information
          sprintf(c, "%0.4f,%0.4f,%0.4f", buff[0], buff[1], buff[2]);
        }
        dataFile.println(String(c));
      }
    }
    dataFile.close();
  }
  else
  {
    //Serial.println("Can't open file for writing");
  }

#ifdef USE_EIC_INTERRUPT
  NVIC_EnableIRQ(EIC_IRQn);
#endif

}




static int ledOnCount = 0;




void trigger(void)
{
    circ.Trigger();

    // Set the LED on when trigger occurs
    digitalWrite(LED_PIN, HIGH);
    ledOnCount = 500;
}



void sample(void)
{
    if (ledOnCount > 0)
    {
        ledOnCount --;
        if (ledOnCount == 0)
        {
            digitalWrite(LED_PIN, LOW);
        }
    }
}





///////////////////////////////////////////////////////////////////////
/////// Interrupt-triggered SPI (for LSM6 only)     ////////////  /////


#include "AccelLsm6.h"


void EIC_Handler (void)
{
    intCount ++;
  
    if (intCount <= 150)  // expect about 107 readings per second. Place a limit
    {

        float readings [3];

        (void) theAccel->getSingleReading(readings);
    
        if (theAccel->useLinearFifo())
        {
            addToLinearFifo(readings);

            avgs.Add(readings);

            sample();

            if (avgs.IsTriggered())
            {
                trigger();
            }
        }
        else
        {
            // Nothing defined here. EIC reading is currently only supported when using the linear FIFO (i.e.
            //  with LSM6).
            gotReadings ++;
        }
    }
    if (intCount > 3000)
    {
        NVIC_DisableIRQ(EIC_IRQn); // "Panic button". Stop if we get too many of these interrupts
    }

    EIC->INTFLAG.bit.EXTINT10 = 1;  // clear flag
    NVIC_ClearPendingIRQ(EIC_IRQn);

}

// Enable input D1 (SAMD PA10) as an interrupt source.
void enableInterrupt(void)
{
    PM->APBAMASK.bit.EIC_ = 1;
    EIC->CTRL.bit.ENABLE = 1;
    while (EIC->STATUS.bit.SYNCBUSY) ;
    EIC->CONFIG[1].bit.SENSE2 = EIC_CONFIG_SENSE2_HIGH_Val; // I10. 8*1+2 = 10

    PORT->Group[0].PINCFG[10].bit.PMUXEN = 1;
    PORT->Group[0].PMUX[5].bit.PMUXE = PORT_PMUX_PMUXE_A_Val;  // I10. 2*5+ (even) = 10. Mux to A = "EXTINT"
  
    EIC->INTENSET.bit.EXTINT10 = 1;
    //EIC->WAKEUP.bit.WAKEUPEN10 = 1;
    NVIC_ClearPendingIRQ(EIC_IRQn);
    NVIC_EnableIRQ(EIC_IRQn);
}


// Disable input D1 (SAMD PA10) as an interrupt source.
// Not used.
void disableInterrupt(void)
{
    // Not tested!!
    EIC->INTENCLR.bit.EXTINT10 = 1;
    NVIC_DisableIRQ(EIC_IRQn);

    EIC->CTRL.bit.ENABLE = 0;
    PM->APBAMASK.bit.EIC_ = 0;
}



///////////////////////////////////////////////////////////////////////////////
/////////// Main program  /////////////////////////////////////////////////////

void setup() {

  
  // Open serial communications and wait for port to open:
#ifdef USE_SERIAL
  Serial.begin(9600);
  delay(2000);  // wait for serial port to connect. Needed for native USB port only
  if (!Serial)
  {
    use_serial = false;
  }
  else
  {
    use_serial = true;
    Serial.println("Serial opened at 9600 baud.");

    // Enter command-line interface for setting/reading the RTC.
    if (DoRtcCLI())
    {
      // Now reset to start again.
      NVIC_SystemReset();
    }

  }
#else
  delay(2000);
  use_serial = false;
#endif


  delay (6000);

  RtcBegin();

  if (use_serial) Serial.print("Initializing SD card...");

  // Set up LED output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);  // start off

  // Set up card detect
  if (cardDetectAvailable())
  {
    pinMode(cardDetect, INPUT_PULLUP);
  }

  // Ensure accel chip select is firmly off.
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);

  pinMode(9, INPUT);  // D9 = AIN7 (VBAT)


  if (use_serial) Serial.println("");
  if (use_serial) Serial.print("Device ID Register = ");
  if (use_serial) Serial.println(DSU->DID.reg, HEX);  // Reads 0x10010305.
  // (    SAMD32G18A, 256KB Flash, 32KB RAM, 48 pin package
  //         Revision 3, Die 0, Cortex-M0+).


  if (boardIsAdxl())
  {
      theAccel = &theAccelAdxl;
  }
  else
  {
      // Unknown device
      theAccel = NULL;
  }

  SD.begin(chipSelect);

  if (use_serial) Serial.println("Begin reading config...");

  if (SD.exists("setup.txt"))
  {

    if (use_serial) Serial.println("Found file");

    File dataFile = SD.open("setup.txt", FILE_READ);
    if (dataFile)
    {

      if (use_serial) Serial.println("Opened setup.txt file for reading");

      char c[20];
      char d[30];
      while (sdReadConfigPair(dataFile, c, 20, d, 30))
      {
        if (strcmp(c, "TRIGGER") == 0)
        {
          setTrigger(String(d).toInt());
        }
        //if (strcmp(c, "DATETIME") == 0)
        //{
          //parseDateTimeToRtc(d);
        //}
        if (strcmp(c, "FREQ") == 0)
        {
          theAccel->setFreq(String(d).toInt());
        }
      }
      dataFile.close();
    }

    //if (use_serial) Serial.print("Copying file ...");

    //sdCopyFile("setup.txt", "setupold.txt");

    //if (use_serial) Serial.println(" done.");

    //    SD.remove("setup.txt");
  }

  initialiseRtc();
  displayRtc(use_serial);
  

  Setup_TC4(theAccel->getTimerReload());

  if (use_serial)
  {
    Serial.print("Using sampling frequency of : ");
    Serial.print(theAccel->getFrequency(), DEC);
    Serial.println(" Hz");
  }

  theAccel->init();

  if (!cardDetectAvailable() || digitalRead(cardDetect))
  {
    writeToLog(0);
  }


  avgs.Init();
  avgs.SetTrigger(1.0 * (float)getTrigger());

  if (use_serial)
  {
    Serial.print("Using trigger level of : ");
    Serial.println(1.0 * (float)getTrigger());
  }



  circ.Init();

#if defined USE_EIC_INTERRUPT
  NVIC_SetPriority(EIC_IRQn, 3);
  enableInterrupt();
#endif

}

extern volatile int gotTimeInterrupt;

// Don't need to check the heartbeat on every timer tick. Divide it down by a certain
//  fraction -- the exact division is not very important.
static unsigned int heartbeatCheckDivisor = 0;
static int heartbeatLastHour = -1;

void loop() {
    // to run repeatedly:
  
  
    if (gotTimeInterrupt)
    {
        // Clear the flag immediately. In theory it should be done in a critical section, but
        //  it should be slow enough that it never matters.
        gotTimeInterrupt = 0;
        heartbeatCheckDivisor ++;

        // Disable the interrupts while we're accessing the linear FIFO
        NVIC_DisableIRQ(EIC_IRQn);

        if (theAccel->useLinearFifo())
        {
            zapLinearFifo(&addToCircBuffer);
        }
        else
        {
            theAccel->zapFifo(&addToCircBufferNoLinearFifo);
        }


        int x  = gotReadings;
        gotReadings = 0;


        int y = intCount;
        intCount = 0;


        if (use_serial)
        {
            Serial.print ("Readings: ");
            Serial.print (x);
            Serial.print(",  Interrupts: ");
            Serial.println(y);
        }

#ifdef USE_EIC_INTERRUPT
    /*
    if (y == 0 && digitalRead(1))
    {
      // Kick-start
      NVIC_SetPendingIRQ(EIC_IRQn);
    }
    */
    NVIC_EnableIRQ(EIC_IRQn);
#endif  // USE_EIC_INTERRUPT


  }
  __DSB();
  __ISB();

  if (heartbeatCheckDivisor >= 16)
  {
    uint8_t currentHour = RtcGetHours();
    
    heartbeatCheckDivisor = 0;
    if (heartbeatLastHour == -1)
    {
      // Don't send a heartbeat just after turning on.
      heartbeatLastHour = currentHour;
    }
    else
    {
      if (heartbeatLastHour != currentHour)
      {
        heartbeatLastHour = currentHour;
        writeToLog(2);
      }
    }

    if (accessesWithoutReadings > 10)
    {
      // Something has gone wrong with the accelerometer. Reset it.

      theAccel->softwareReset();
      theAccel->init();
      avgs.Init();

      circ.Init();

#ifdef USE_LINEAR_FIFO
      clearLinearFifo();
#endif

      writeToLog(3);
    }
  }

  if (circ.IsFullSample())
  {
    // The circular buffer has been filled for us to zap out the data.


    writeToLog(1);

    // Re-start buffering

    // We can ignore any interrupts that have occurred while we've been busy
    gotTimeInterrupt = 0;

  }

    if (!gotTimeInterrupt && !haveFullSample)
    {
        SCB->SCR = ~(SCB_SCR_SLEEPDEEP_Msk | SCB_SCR_SLEEPONEXIT_Msk);   // only IDLE mode. We want to be able to wake fast.
        // Don't use SLEEP ON EXIT mode.

        __DSB();
        __ISB();
        __WFI();
    }

}
