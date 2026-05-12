/**
 * @file lora.h
 *
 */

#ifndef LORA_H
#define LORA_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct {
  char appName[NAME_LEN];
  char title[TITLE_LEN];
  char body[BODY_LEN];
} LoraNotification;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
 void setupLora();
 void addAlertToLora(singleAlert alert);
 void processNewAlertsToLora(int oldMinIdx, int newMinIdx, int oldMaxIdx, int newMaxIdx);
 void settingLoRaParams();

/**********************
 *      MACROS
 **********************/

#endif /*LORA_H*/
