#include <lvgl.h>
#include <LilyGoLib.h>

#include "settings.h"

lilygoSettings setSetting() {
  lilygoSettings settings;

  settings.cityID = CITY_ID;
  settings.weatherAPIKey = OPENWEATHERMAP_APIKEY;
  settings.weatherURL = OPENWEATHERMAP_URL;
  settings.weatherUnit = TEMP_UNIT;
  settings.weatherLang = TEMP_LANG;
  settings.weatherUpdateInterval = WEATHER_UPDATE_INTERVAL;
  settings.ntpServer = NTP_SERVER;
  settings.gmtOffset = GMT_OFFSET_SEC;
  settings.dstOffset = 0;
  settings.geoipURL = GEOIP_URL;
  settings.locationUpdateInterval = LOCATION_UPDATE_INTERVAL;
  settings.wifiSSID = WIFI_SSID;
  settings.wifiPwd = WIFI_PWD;
  settings.totpAccount = TOTP_ACCOUNT;
  settings.totpSecret = TOTP_SECRET;
  settings.loraMagic = LORA_MAGIC;

  return settings;
}