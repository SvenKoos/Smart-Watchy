/**
 * @file weatherData.h
 *
 */

#ifndef WEATHER_DATA_H
#define WEATHER_DATA_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/
typedef struct weatherData {
  int8_t temperature;
  int16_t weatherConditionCode;
  bool isMetric;
  char weatherDescription[NAME_LEN];
  char name[NAME_LEN];
  long offset;
  char log[LOG_LEN];
  int code;
} weatherData;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
weatherData getWeatherData(String cityID, String units, String lang, String url, String apiKey, uint8_t updateInterval);
weatherData getWeatherDataExt(double latitude, double longitude);
weatherData getWeatherDataByLocation(double latitude, double longitude, String units, String lang, String url, String apiKey, uint8_t updateInterval);
String Normalize2ASCII(String source);

/**********************
 *      MACROS
 **********************/

#endif /*WEATHER_DATA_H*/
