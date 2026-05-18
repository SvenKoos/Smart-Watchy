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
  uint32_t magic;
  char appName[NAME_LEN];
  char title[TITLE_LEN];
  char body[BODY_LEN];
} EncryptedPayload;

typedef struct {
    uint32_t packetCounter; // Bleibt unverschlüsselt (wird für den IV benötigt)
    EncryptedPayload data;  // Wird verschlüsselt
} LoraNotification;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
 void setupLora();
 void addMsgToLora(const char* msg);
 void addAlertToLora(singleAlert alert);
 void processNewAlertsToLora(int oldMinIdx, int newMinIdx, int oldMaxIdx, int newMaxIdx);
 void settingLoRaParams();
 void transmitAlertsToLora();

/**********************
 *      MACROS
 **********************/

#endif /*LORA_H*/
