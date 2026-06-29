/**
 * @file powerData.h
 *
 */

#ifndef POWER_DATA_H
#define POWER_DATA_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct powerData {
  bool isCharging;
  bool isDischarge;
  bool isUSBPlugin;
  uint16_t battVoltagemV;
  uint16_t vBusVoltagemV;
  uint16_t systemVoltagemV;
  uint8_t batteryPercent;
  char log[LOG_LEN];
  int code;
} powerData;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
powerData getPowerData();
void setupPowerMgt();
void verifyPowerMgt();
void resetBatteryCalibration();
uint8_t getCustomBatteryPercent(uint16_t mv);

/**********************
 *      MACROS
 **********************/

#endif /*POWER_DATA_H*/
