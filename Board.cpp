#include "Board.h"




#define UNIQUE_SERIAL_NUMBER_0         (*((const unsigned int * const)0x0080A00C))
#define UNIQUE_SERIAL_NUMBER_1         (*((const unsigned int * const)0x0080A040))
#define UNIQUE_SERIAL_NUMBER_2         (*((const unsigned int * const)0x0080A044))
#define UNIQUE_SERIAL_NUMBER_3         (*((const unsigned int * const)0x0080A048))


typedef enum
{
    DEFINITION_ADXL355 = 0
  
} ACCEL_DEFINITION_T;


typedef struct
{
    unsigned int serial_number [4];   // The serial number which this applies to. If all zeros, is the default
    ACCEL_DEFINITION_T accelDef;

  
} DEVICE_DEFINITION_T;



const static DEVICE_DEFINITION_T boardDefinitions[] = 
{
    {
        {0xF5DAD2B9 ,  0x504D5741 ,  0x372E3120 ,  0xFF132043},
        DEFINITION_ADXL355
    },
};


bool   boardIsAdxl(void)
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


