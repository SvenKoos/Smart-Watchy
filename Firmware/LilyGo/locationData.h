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
  double latitudeGoogle;
  double longitudeGoogle;
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
locationData getLocationData(String url);
locationData getLocationDataGoogle(String geoLocURL, String googleApiKey, String jsonScan);
void setupLocationData();
String discoverWiFiNetworks();

/**********************
 *      MACROS
 **********************/

#endif /*LOCATION_DATA_H*/
