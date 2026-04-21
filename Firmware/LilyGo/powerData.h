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
  uint8_t battVoltagemV;
  uint8_t vBusVoltagemV;
  uint8_t systemVoltagemV;
  uint8_t batteryPercent;
  char log[LOG_LEN];
  int code;
} powerData;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
powerData getPowerData();

/**********************
 *      MACROS
 **********************/

#endif /*POWER_DATA_H*/
