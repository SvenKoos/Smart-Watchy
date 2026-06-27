/**
 * @file alertData.h
 *
 */

#ifndef ALERT_DATA_H
#define ALERT_DATA_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct alertData {
  int count;
  char log[LOG_LEN];
  int code;
} alertData;

typedef struct singleAlert {
  char appName[NAME_LEN];
  char title[TITLE_LEN];
  char body[BODY_LEN];
  char timeStamp[TIMESTAMP_LEN];
  bool dismissed;
  int id;
} singleAlert;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
alertData getAlertData(const String gatewayIP, const String macAdress);
void vibMotor();
String cleanNotificationText(String source);

/**********************
 *      MACROS
 **********************/

#endif /*ALERT_DATA_H*/
