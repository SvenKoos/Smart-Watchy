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
#include <WiFiManager.h>

#include "dataCollection.h"
#include "config.h"
#include "settings.h"
#include "locationData.h"
#include "weatherData.h"
#include "alertData.h"
#include "powerData.h"
#include "accellData.h"
#include "lora.h"

extern lilygoSettings settings;

extern bool WIFI_CONFIGURED;
extern bool WIFI_CONNECTED;

extern weatherData currentWeather;
extern locationData currentLocation;
extern alertData currentAlerts;
extern powerData currentPower;
extern accellData currentAccelleration;

void setupDataCollection() {
  WiFi.persistent(false);
}

void collectData(void) {
  String localIP;
  String gatewayIP;
  String macAdress;

  if (connectWiFi(localIP, gatewayIP, macAdress)) {
    WIFI_CONNECTED = true;

    // get location data
    currentLocation = getLocationData(settings.geoipURL, settings.locationUpdateInterval);
    if (currentLocation.code == CODE_NO_ERROR) {
      // get IP data
      strncpy(currentLocation.localIP, localIP.c_str(), sizeof(currentLocation.localIP) - 1);
      currentLocation.localIP[sizeof(currentLocation.localIP) - 1] = '\0';
      strncpy(currentLocation.gatewayIP, gatewayIP.c_str(), sizeof(currentLocation.gatewayIP) - 1);
      currentLocation.gatewayIP[sizeof(currentLocation.gatewayIP) - 1] = '\0';

      // get weather data
      currentWeather = getWeatherDataExt(currentLocation.latitude, currentLocation.longitude);
    } else {
      currentWeather = getWeatherData(settings.cityID, settings.weatherUnit,
                                      settings.weatherLang, settings.weatherURL,
                                      settings.weatherAPIKey, settings.weatherUpdateInterval);
    }

    // get alert data
    currentAlerts = getAlertData(gatewayIP, macAdress);

    disconnectWifi();
  } else
    WIFI_CONNECTED = false;

  // get accelleration data
  currentAccelleration = getAccellData();

  // get power data
  currentPower = getPowerData();

  // handle the Lora queue
  transmitAlertsToLora();
}

bool connectWiFi(String &hostIP, String &gatewayIP, String &macAdress) {
  Serial.println("connectWiFi Start");
  Serial.print("SSID ");
  Serial.println(settings.wifiSSID);
  Serial.print("Pwd ");
  Serial.println(settings.wifiPwd);

  WiFi.setSleep(false);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (WL_CONNECT_FAILED == WiFi.begin(settings.wifiSSID, settings.wifiPwd)) {  // WiFi not setup: WiFi.begin(),
                                                                               // you can also use hard coded credentials: WiFi.begin(SSID,PASS);
    WIFI_CONFIGURED = false;

    Serial.print("Wifi failed: ");
    Serial.println(WiFi.status(), DEC);

    // turn off radios
    disconnectWifi();
  } else {
    if (WL_CONNECTED == WiFi.waitForConnectResult()) {  // attempt to connect for 10s
      WIFI_CONFIGURED = true;
      hostIP = WiFi.localIP().toString();
      gatewayIP = WiFi.gatewayIP().toString();
      macAdress = String(WiFi.macAddress());

      Serial.print("connectWiFi Host Gateway MAC ");
      Serial.print(hostIP);
      Serial.print(" ");
      Serial.print(gatewayIP);
      Serial.print(" ");
      Serial.println(macAdress);
    } else {  // connection failed, time out
      WIFI_CONFIGURED = false;

      Serial.print("Connection failed: ");
      Serial.println(WiFi.status(), DEC);

      // turn off radios
      disconnectWifi();
    }

    if ((hostIP.length() == 0) || (gatewayIP.length() == 0) || (macAdress.length() == 0)) {
      disconnectWifi();
    }
  }

  return WIFI_CONFIGURED;
}

void disconnectWifi() {
  Serial.println("disconnectWifi Start");

  // turn off radios
  WIFI_CONFIGURED = false;
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}
