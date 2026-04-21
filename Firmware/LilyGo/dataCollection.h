/**
 * @file dataCollection.h
 *
 */

#ifndef DATA_COLLECTION_H
#define DATA_COLLECTION_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
#define NAME_LEN 32
#define IP_LEN 64
#define TITLE_LEN 64
#define LOG_LEN 128
#define BODY_LEN 128
#define MESSAGE_LEN 24
#define TIMESTAMP_LEN 24
#define ALERT_MAX_NO 20
#define MAX_ACCEL_QUIET 5

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
void collectData(void);
bool connectWiFi(String &hostIP, String &gatewayIP, String &macAdress);
void disconnectWifi();
bool setupWifi();
void configModeCallback(WiFiManager *myWiFiManager);

/**********************
 *      MACROS
 **********************/

#endif /*DATA_COLLECTION_H*/
