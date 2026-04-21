/**
 * @file locationData.h
 *
 */

#ifndef LOCATION_DATA_H
#define LOCATION_DATA_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

typedef struct locationData {
  double latitude;
  double longitude;
  char city[NAME_LEN];
  char cityShort[NAME_LEN];
  char publicIP[IP_LEN];
  char localIP[IP_LEN];
  char gatewayIP[IP_LEN];
  long offset;
  char log[LOG_LEN];
  int code;
} locationData;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
locationData getLocationData(String url, uint8_t updateInterval);

/**********************
 *      MACROS
 **********************/

#endif /*LOCATION_DATA_H*/
