#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>
#include <Time.h>
#include <TimeLib.h>
#include <esp_sntp.h>

#include "dataCollection.h"
#include "weatherData.h"
#include "locationData.h"
#include "config.h"
#include "settings.h"
#include "syncNTP.h"

RTC_DATA_ATTR int locationIntervalCounter = -1;
extern locationData currentLocation;

locationData getLocationData(String url, uint8_t updateInterval) {
  Serial.println("getLocationData Start");

  currentLocation.code = CODE_NO_ERROR;

  if (locationIntervalCounter < 0) {  //-1 on first run, set to updateInterval
    locationIntervalCounter = updateInterval;
  }
  if (locationIntervalCounter >= updateInterval) {  // only update if UPDATE_INTERVAL has elapsed
                                                    // i.e. 30 minutes
    Serial.println("getLocationData Get");

    HTTPClient http;                                // Use location API if WiFi is connected
    http.setConnectTimeout(3000);                   // 3 second max timeout
    String locationQueryURL = url;
    http.begin(locationQueryURL.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String payload = http.getString();
      JSONVar responseObject = JSON.parse(payload);
      if ((!responseObject.hasOwnProperty("ip")) || (!responseObject.hasOwnProperty("city")) || (!responseObject.hasOwnProperty("latitude")) || (!responseObject.hasOwnProperty("longitude")) || (!responseObject.hasOwnProperty("timezone"))) {
        currentLocation.code = CODE_PARSE_ERROR;
        strncpy(currentLocation.log, "Missing fields", sizeof(currentLocation.log) - 1);
        currentLocation.log[sizeof(currentLocation.log) - 1] = '\0';

        return currentLocation;
      }

      const char* ip = (const char*)responseObject["ip"];
      if (ip != nullptr) {
        strncpy(currentLocation.publicIP, ip, sizeof(currentLocation.publicIP) - 1);
        currentLocation.publicIP[sizeof(currentLocation.publicIP) - 1] = '\0';
      }
      const char* city = (const char*)responseObject["city"];
      if (city != nullptr) {
        strncpy(currentLocation.city, city, sizeof(currentLocation.city) - 1);
        currentLocation.city[sizeof(currentLocation.city) - 1] = '\0';
      }
      currentLocation.latitude = double(responseObject["latitude"]);
      currentLocation.longitude = double(responseObject["longitude"]);
      currentLocation.offset = long(responseObject["timezone"]["offset"]);

      String cityString = Normalize2ASCII(String(currentLocation.city));
      strncpy(currentLocation.city, cityString.c_str(), sizeof(currentLocation.city) - 1);
      currentLocation.city[sizeof(currentLocation.city) - 1] = '\0';

      // create short city name for display
      String name;
      int maxNameLength = 10;
      if (strlen(currentLocation.city) > maxNameLength) {
        name = String(currentLocation.city, maxNameLength - 1);
        if (name[maxNameLength - 2] != ' ') {
          name = name + String(".");
        }
      } else
        name = currentLocation.city;
      strcpy(currentLocation.cityShort, name.c_str());

      Serial.print("getLocationData City: "); Serial.println(currentLocation.city);
    } else {
      // http error
      currentLocation.code = CODE_HTTP_ERROR;

      Serial.print("getLocationData Error code: "); Serial.println(currentLocation.code, DEC);
    }
    strncpy(currentLocation.log, String(httpResponseCode).c_str(), sizeof(currentLocation.log) - 1);
    currentLocation.log[sizeof(currentLocation.log) - 1] = '\0';

    http.end();

    locationIntervalCounter = 0;
  } else {
    locationIntervalCounter++;
  }
  return currentLocation;
}
