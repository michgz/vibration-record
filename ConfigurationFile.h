

#include "Arduino.h"

#include <SD.h>   // for File

#include <stdint.h>
#include <stdbool.h>


extern bool sdReadConfigPair(File f, char * buf1, int len1, char * buf2, int len2);
extern void sdCopyFile(String nameFrom, String nameTo);

