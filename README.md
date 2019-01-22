# vibration-record

Records short (5 second - 20 second) vibration patterns as measured by an accelerometer, and stores them on an SD card. Recordings are triggered by signals larger than a threshold (deviation from the long-term average).

Uses the Arduino IDE and the SAMD21 processor.

Also has post-processing software based on Python 2.7.

## Reference hardware

* EITHER, Option 1: 
  * Adafruit Feather M0 Basic Proto
  * Adalogger Featherwing - RTC + SD
* OR, Option 2:
  * Adalogger Feather M0 with SD card
  * RTC break-out board
  
Also need:
  
* Accelerometer break-out board. Either:
  * ADXL355, or
  * LSM6DS3
