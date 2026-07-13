#ifndef SETTINGS_H
#define SETTINGS_H

#define OPENWEATHERMAP_APIKEY "--------------------------------"
#define OPENWEATHERMAP_URL "https://api.openweathermap.org/data/4.0/onecall/current?exclude=minutely,hourly,daily,alerts"  //open weather api
#define TEMP_UNIT "metric"                                                   //metric = Celsius , imperial = Fahrenheit
#define TEMP_LANG "en"
#define NTP_SERVER "pool.ntp.org"
#define GEOIP_URL "http://ipwho.is"  //  GeoIP (Get IP address location in JSON format)
#define LOCATION_UPDATE_INTERVAL 15
#define WIFI_SSID "--------"
#define WIFI_PWD "--------"
#define TOTP_ACCOUNT "----------------"
#define TOTP_SECRET "----------------"
#define LORA_MAGIC 0xDEADBEEF
#define BACKGROUND_BLACK true
#define STEP_GOAL 10000
#define GOOGLE_APIKEY "--------------------------------"
#define GOOGLE_LOCATION_URL "https://www.googleapis.com/geolocation/v1/geolocate"
#define REVERSE_LOCATION_URL "https://api.openweathermap.org/geo/1.0/reverse"

typedef struct lilygoSettings {
  // Weather Settings
  String weatherAPIKey;
  String weatherURL;
  String weatherUnit;
  String weatherLang;
  // NTP Settings
  String ntpServer;
  int dstOffset;
  // location settings
  String geoipURL;
  int8_t locationUpdateInterval;
  // WiFi settings
  String wifiApSSID;
  String wifiSSID;
  String wifiPwd;
  // TOTP settings
  String totpAccount;
  String totpSecret;
  // General settings
  uint32_t loraMagic;
  bool displayBGrndBlack;
  int16_t stepGoal;
  // Google settings
  String googleApiKey;
  String googleGeoLocationURL;
  String weatherReverseLocationURL;
} lilygoSettings;

lilygoSettings setSetting();
void setTheme(bool inverted);
void updateIconTheme(lv_obj_t * icon_obj, bool inverted);
lv_color_t GetTheme(int themeID);

#endif
