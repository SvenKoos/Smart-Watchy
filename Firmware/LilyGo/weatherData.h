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
  long offset;
  char weatherIcon[NAME_LEN];
  char log[LOG_LEN];
  int code;
  uint32_t currentDT;
  uint32_t currentSunrise;
  uint32_t currentSunset;
  double uvi;
} weatherData;

/**********************
 * GLOBAL PROTOTYPES
 **********************/
weatherData getWeatherDataByLocation(double latitude, double longitude, String units, String lang, String url, String apiKey);
void setupWeatherData();
String Normalize2ASCII(String source);
locationData getReverseLocation(double latitude, double longitude, String url, String apiKey);

/**********************
 *      MACROS
 **********************/

#endif /*WEATHER_DATA_H*/
