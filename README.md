# vibration-record

Records short (<5 second) vibration patterns as measured by an accelerometer, and stores them on an SD card. Recordings are triggered by signals larger than a threshold (deviation from the long-term average).

Uses the Arduino IDE and the SAMD21 processor.

## Reference hardware

* Adafruit Feather M0 Basic Proto
* Adalogger Featherwing - RTC + SD
* Accelerometer break-out board. Either:
  * ADXL355, or
  * LSM6DS3
