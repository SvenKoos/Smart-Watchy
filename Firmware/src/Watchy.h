#ifndef WATCHY_H
#define WATCHY_H

#include <Arduino.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Arduino_JSON.h>
#include <GxEPD2_BW.h>
#include <Wire.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include "DSEG7_Classic_Bold_53.h"
#include "WatchyRTC.h"
#include "BLE_OTA.h"
#include "bma.h"
#include "config.h"

// SvKo added
#include <Time.h>
#include <chrono>
#include <bits/stdc++.h>

// SvKo added
#include "BLE_Bond.h"

// SvKo
#define NAME_LEN 32
#define IP_LEN 16
#define TITLE_LEN 64
#define LOG_LEN 32
#define BODY_LEN 128
#define MESSAGE_LEN 24
#define TIMESTAMP_LEN 24

// SvKo
#define ALERT_MAX_NO 20

// SvKo
#define MAX_ACCEL_QUIET 5

typedef struct weatherData {
  int8_t temperature;
  int16_t weatherConditionCode;
  bool isMetric;
  // SvKo
  // String weatherDescription;
  char weatherDescription[NAME_LEN];
  // SvKo: added
  char name[NAME_LEN];
  long offset;
  char log[LOG_LEN];
  int code;
} weatherData;

  // SvKo: added
typedef struct locationData {
  double latitude;
  double longitude;
  char city[NAME_LEN];
  char publicIP[IP_LEN];
  char localIP[IP_LEN];
  char gatewayIP[IP_LEN];
  long offset;
  char log[LOG_LEN];
  int code;  
} locationData;

  // SvKo: added
typedef struct alertData {
  JSONVar alerts;
  char log[LOG_LEN];
  int code;  
} alertData;

  // SvKo: added
typedef struct accelData {
  uint32_t stepCounter;
  int16_t accelX;
  int16_t accelY;
  int16_t accelZ;
  uint8_t direction;
  bool move;
  int code;  
  char log[LOG_LEN];
} accelData;

typedef struct singleAlert {
  char appName[NAME_LEN];
  char title[TITLE_LEN];
  char body[BODY_LEN];
  char timeStamp[TIMESTAMP_LEN];
  bool dismissed;
  int id;
} singleAlert;

typedef struct watchySettings {
  // Weather Settings
  String cityID;
  String weatherAPIKey;
  String weatherURL;
  String weatherUnit;
  String weatherLang;
  int8_t weatherUpdateInterval;
  // NTP Settings
  String ntpServer;
  int gmtOffset;
  int dstOffset;
  // SvKo added
  String geoipURL;
  int8_t locationUpdateInterval;
} watchySettings;

class Watchy {
public:
  static WatchyRTC RTC;
  static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;
  tmElements_t currentTime;
  watchySettings settings;

public:
  explicit Watchy(const watchySettings &s) : settings(s) {} // constructor
  void init(String datetime = "");
  void deepSleep();
  static void displayBusyCallback(const void *);
  float getBatteryVoltage();
  void vibMotor(uint8_t intervalMs = 100, uint8_t length = 20);

  void handleButtonPress();
  void showMenu(byte menuIndex, bool partialRefresh);
  void showFastMenu(byte menuIndex);
  void showAbout();
  void showBuzz();
  void showAccelerometer();
  void showUpdateFW();
  void showSyncNTP();
  bool syncNTP();
  bool syncNTP(long gmt);
  bool syncNTP(long gmt, String ntpServer);
  void setTime();
  void setupWifi();
  // SvKo change
  bool connectWiFi(String &hostIP, String &gatewayIP, String &macAdress);
  
  weatherData getWeatherData();
  weatherData getWeatherData(String cityID, String units, String lang,
                             String url, String apiKey, uint8_t updateInterval);
  // SvKo added
  weatherData getWeatherDataExt(double latitude, double longitude);
  weatherData getWeatherDataByLocation(double latitude, double longitude, String units, String lang, String url, String apiKey, uint8_t updateInterval);

  void updateFWBegin();

  void showWatchFace(bool partialRefresh);
  virtual void drawWatchFace(); // override this method for different watch
                                // faces
  // SvKo: added
  locationData getLocationData();
  locationData getLocationData(String url, uint8_t updateInterval);
  
  // SvKo added
  void showAlert(singleAlert alert, int index, int amount);
  alertData getAlertData(bool darkMode);

  // SvKo added
  accelData getAccelData();
  
  // SvKo
  void bleSetup();
  void bondBLE();

private:
  void _bmaConfig();
  static void _configModeCallback(WiFiManager *myWiFiManager);
  static uint16_t _readRegister(uint8_t address, uint8_t reg, uint8_t *data,
                                uint16_t len);
  static uint16_t _writeRegister(uint8_t address, uint8_t reg, uint8_t *data,
                                 uint16_t len);
  String Unicode2ASCII(String source);
  String Normalize2ASCII(String source);
};

extern RTC_DATA_ATTR int guiState;
extern RTC_DATA_ATTR int menuIndex;
extern RTC_DATA_ATTR BMA423 sensor;
extern RTC_DATA_ATTR bool WIFI_CONFIGURED;
extern RTC_DATA_ATTR bool BLE_CONFIGURED;

#endif
