#include "Board.h"




#define UNIQUE_SERIAL_NUMBER_0         (*((const unsigned int * const)0x0080A00C))
#define UNIQUE_SERIAL_NUMBER_1         (*((const unsigned int * const)0x0080A040))
#define UNIQUE_SERIAL_NUMBER_2         (*((const unsigned int * const)0x0080A044))
#define UNIQUE_SERIAL_NUMBER_3         (*((const unsigned int * const)0x0080A048))


typedef enum
{
    DEFINITION_ADXL355 = 0,
    DEFINITION_LSM6DS3 = 1,
    DEFINITION_ADXL335 = 2

    // Note the difference between ADXL335 and ADXL355! -- the first is a cheap analogue MEMS sensor,
    //  the second is a costly digital-interface ceramic sensor.

} ACCEL_DEFINITION_T;

const static ANALOGUE_ACCEL_DEFINITION_T  accelAnalogueDefinitions [3] =
{
    {false, 0.0, 0.0},      // ADXL355
    {false, 0.0, 0.0},      // LSM6DS
    {true,  0.0, 1.0}       // ADXL335

};

// Currently supports two types of hardware: the Feather M0 Adalogger (https://www.adafruit.com/product/2796)
//  or the Featherwing Adalogger (https://www.adafruit.com/product/2922) together with any M0 Feather.
typedef enum
{
    DEFINITION_FEATHER_ADALOGGER = 0,
    DEFINITION_FEATHERWING_ADALOGGER = 1
  
} LOGGER_DEFINITION_T;



typedef struct
{
    unsigned int serial_number [4];   // The serial number which this applies to.
    ACCEL_DEFINITION_T accelDef;
    LOGGER_DEFINITION_T loggerDef;

} DEVICE_DEFINITION_T;



const static DEVICE_DEFINITION_T boardDefinitions[] = 
{
    {
        {0xF5DAD2B9 ,  0x504D5741 ,  0x372E3120 ,  0xFF132043},
        DEFINITION_ADXL355,
        DEFINITION_FEATHER_ADALOGGER
    },

/*
    {
        {0x50E910B8 ,  0x504D5741 ,  0x372E3120 ,  0xFF19232E},
        DEFINITION_LSM6DS3,
        DEFINITION_FEATHERWING_ADALOGGER
    }
*/
    {
        {0x50E910B8 ,  0x504D5741 ,  0x372E3120 ,  0xFF19232E},
        DEFINITION_ADXL335,
        DEFINITION_FEATHERWING_ADALOGGER
    }

};


bool   boardIsAdxl(void)  // True if ADXL355
{
    int i;

    for (i = 0; i < sizeof(boardDefinitions)/sizeof(boardDefinitions[0]); i++)
    {
        if (     (boardDefinitions[i].serial_number[0] == UNIQUE_SERIAL_NUMBER_0)
              && (boardDefinitions[i].serial_number[1] == UNIQUE_SERIAL_NUMBER_1)
              && (boardDefinitions[i].serial_number[2] == UNIQUE_SERIAL_NUMBER_2)
              && (boardDefinitions[i].serial_number[3] == UNIQUE_SERIAL_NUMBER_3)   )
        {
            return boardDefinitions[i].accelDef == DEFINITION_ADXL355;
        }
    }

    return false;
  
}

bool   boardIsAnalogue(void)
{
    int i;

    for (i = 0; i < sizeof(boardDefinitions)/sizeof(boardDefinitions[0]); i++)
    {
        if (     (boardDefinitions[i].serial_number[0] == UNIQUE_SERIAL_NUMBER_0)
              && (boardDefinitions[i].serial_number[1] == UNIQUE_SERIAL_NUMBER_1)
              && (boardDefinitions[i].serial_number[2] == UNIQUE_SERIAL_NUMBER_2)
              && (boardDefinitions[i].serial_number[3] == UNIQUE_SERIAL_NUMBER_3)   )
        {
            return boardDefinitions[i].accelDef == DEFINITION_ADXL335;
        }
    }

    return false;
}

bool boardIsFeatherAdalogger(void)
{
    int i;

    for (i = 0; i < sizeof(boardDefinitions)/sizeof(boardDefinitions[0]); i++)
    {
        if (     (boardDefinitions[i].serial_number[0] == UNIQUE_SERIAL_NUMBER_0)
              && (boardDefinitions[i].serial_number[1] == UNIQUE_SERIAL_NUMBER_1)
              && (boardDefinitions[i].serial_number[2] == UNIQUE_SERIAL_NUMBER_2)
              && (boardDefinitions[i].serial_number[3] == UNIQUE_SERIAL_NUMBER_3)   )
        {
            return boardDefinitions[i].loggerDef == DEFINITION_FEATHER_ADALOGGER;
        }
    }

    return true;   // Defaults to true
  
}





