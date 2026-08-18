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

typedef struct agendaItem {
    uint32_t alertID;
    char subject[TITLE_LEN];
    uint64_t startTime; // Unix-Timestamp in Millisekunden (UTC)
    uint64_t endTime;   // Unix-Timestamp in Millisekunden (UTC)
} agendaItem;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
alertData getAlertData(const String gatewayIP, const String macAdress);
void vibMotor();
String cleanNotificationText(String source);
int extractAgendaFromAlerts(int count);
time_t portableTimegm(const tm* tm);
uint64_t parseIsoToUnixMs(const String& timeStr);
void updateAndSortAgenda();
void resetAgenda();

/**********************
 *      MACROS
 **********************/

#endif /*ALERT_DATA_H*/
