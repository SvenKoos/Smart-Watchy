#include "Print.h"
#include "WString.h"
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

RTC_DATA_ATTR int locationIntervalCounter;
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

    HTTPClient http;               // Use location API if WiFi is connected
    http.setConnectTimeout(3000);  // 3 second max timeout
    http.setTimeout(4000);        // Max. 4 Sek auf die eigentlichen JSON-Daten warten
    
    String locationQueryURL = url;
    http.begin(locationQueryURL.c_str());
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      String payload = http.getString();
      JSONVar responseObject = JSON.parse(payload);
      Serial.println(responseObject);

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

        String cityString = Normalize2ASCII(String(currentLocation.city));
        strncpy(currentLocation.city, cityString.c_str(), sizeof(currentLocation.city) - 1);
        currentLocation.city[sizeof(currentLocation.city) - 1] = '\0';

        // create short city name for display
        String name;
        int maxNameLength = 12;
        if (strlen(currentLocation.city) > maxNameLength) {
          name = String(currentLocation.city, maxNameLength - 1);
          if (name[maxNameLength - 2] != ' ') {
            name = name + String(".");
          }
        } else
          name = currentLocation.city;
        strcpy(currentLocation.cityShort, name.c_str());
      } else {
        strcpy(currentLocation.city, "");
        strcpy(currentLocation.cityShort, "");
      }

      currentLocation.latitude = double(responseObject["latitude"]);
      currentLocation.longitude = double(responseObject["longitude"]);
      currentLocation.offset = long(responseObject["timezone"]["offset"]);

      Serial.print("getLocationData City: ");
      Serial.println(currentLocation.city);
    } else {
      // http error
      currentLocation.code = CODE_HTTP_ERROR;

      Serial.print("getLocationData Error code: ");
      Serial.println(currentLocation.code, DEC);
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

String discoverWiFiNetworks(uint8_t updateInterval) {
  // WiFi disconnected!

  if ((locationIntervalCounter == updateInterval) || (locationIntervalCounter < 0)) {  // only update if UPDATE_INTERVAL has elapsed

    // WLANs in der Umgebung scannen
    int n = WiFi.scanNetworks();
    if (n == 0) {
      Serial.println("Keine WLAN-Netzwerke gefunden.");
      return "";
    } else {
      Serial.print(n, DEC);
      Serial.println(" WLAN-Netzwerke gefunden.");
    }

    // JSON Payload mit Arduino_JSON bauen
    JSONVar requestBody;
    requestBody["considerIp"] = "false";  // Als String oder Boolean, Google akzeptiert beides

    JSONVar wifiPoints;
    int maxPoints = min(n, 8);

    for (int i = 0; i < maxPoints; ++i) {
      JSONVar point;
      point["macAddress"] = WiFi.BSSIDstr(i);
      point["signalStrength"] = WiFi.RSSI(i);

      // In Arduino_JSON fügt man Elemente über den Index an ein Array an
      wifiPoints[i] = point;
    }

    requestBody["wifiAccessPoints"] = wifiPoints;

    // JSON in einen String konvertieren
    String jsonString = JSON.stringify(requestBody);

    return jsonString;
  } else
    return "";
}

locationData getLocationDataGoogle(String geoLocURL, String googleApiKey, String jsonScan) {
  // WiFi connected!
  Serial.println("getLocationDataGoogle Start");

  currentLocation.code = CODE_NO_ERROR;

  Serial.println("getLocationDataGoogle Get");

  if (jsonScan.length() > 0) {

    // HTTP POST an Google
    HTTPClient http;
    http.setConnectTimeout(3000);  // 3 second max timeout
    http.setTimeout(4000);        // Max. 4 Sek auf die eigentlichen JSON-Daten warten

    String url = geoLocURL + "?key=" + googleApiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(jsonScan);
    if (httpResponseCode > 0) {
      String response = http.getString();
      // Response parsen mit Arduino_JSON
      JSONVar responseObj = JSON.parse(response);
      Serial.println(responseObj);

      // Überprüfen, ob das Parsen erfolgreich war und das "location" Objekt existiert
      if (JSON.typeof(responseObj) != "undefined" && responseObj.hasOwnProperty("location")) {

        // Werte auslesen (wichtig: explizit nach double casten!)
        double lat = (double)responseObj["location"]["lat"];
        double lng = (double)responseObj["location"]["lng"];
        Serial.printf("\nErfolgreich geortet!\n");

        currentLocation.latitudeGoogle = lat;
        currentLocation.longitudeGoogle = lng;

        strncpy(currentLocation.log, String(httpResponseCode).c_str(), sizeof(currentLocation.log) - 1);
        currentLocation.log[sizeof(currentLocation.log) - 1] = '\0';
      } else {
        Serial.println("Fehler beim Parsen des Google-Responses oder Ortung fehlgeschlagen.");

        currentLocation.code = CODE_PARSE_ERROR;
        strncpy(currentLocation.log, "Parsing error or location failed", sizeof(currentLocation.log) - 1);
        currentLocation.log[sizeof(currentLocation.log) - 1] = '\0';
      }
    } else {
      // http error
      currentLocation.code = CODE_HTTP_ERROR;

      Serial.print("getLocationData Error code: ");
      Serial.println(currentLocation.code, DEC);
      Serial.printf("Fehler beim API-Aufruf: %s (%d)\n", http.errorToString(httpResponseCode).c_str(), httpResponseCode);

      strncpy(currentLocation.log, "http error", sizeof(currentLocation.log) - 1);
      currentLocation.log[sizeof(currentLocation.log) - 1] = '\0';
    }

    http.end();
  }

  return currentLocation;
}

void setupLocationData() {
  locationIntervalCounter = -1;

  currentLocation.latitude = 0;
  currentLocation.longitude = 0;
  currentLocation.offset = 0;
  strcpy(currentLocation.city, "");
  strcpy(currentLocation.cityShort, "");
  currentLocation.latitudeGoogle = 0;
  currentLocation.longitudeGoogle = 0;
}