#ifndef BOARD_H_
#define BOARD_H_

extern bool boardIsAdxl(void);

extern bool boardIsAnalogue(void);

extern bool boardIsFeatherAdalogger(void);

extern bool boardHasMcp9808(void);

typedef struct
{
    bool   isAnalogue;   // Next values only matter if this is true
    float  scale;
    float  offset;
  
} ANALOGUE_ACCEL_DEFINITION_T;



#endif  // BOARD_H_
