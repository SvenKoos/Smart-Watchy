#ifndef SETTINGS_H
#define SETTINGS_H

//Weather Settings
//#define CITY_ID "5128581" //New York City https://openweathermap.org/current#cityid
#define CITY_ID "2660253"  //Interlaken
#define OPENWEATHERMAP_APIKEY "5bcd8b064ea29fe07547ac723e4a2875"
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather"  //open weather api
#define TEMP_UNIT "metric"                                                   //metric = Celsius , imperial = Fahrenheit
#define TEMP_LANG "en"
#define WEATHER_UPDATE_INTERVAL 15  //default 30, must be greater than 5, measured in minutes
//NTP Settings
#define NTP_SERVER "pool.ntp.org"
// #define GMT_OFFSET_SEC 3600 * -5 //New York is UTC -5 EST, -4 EDT
#define GMT_OFFSET_SEC 3600 * 1      //CET
#define GEOIP_URL "http://ipwho.is"  //  GeoIP (Get IP address location in JSON format)
#define LOCATION_UPDATE_INTERVAL 15
#define WIFI_SSID "AndroidAP9339"
#define WIFI_PWD "Lene7890"

typedef struct lilygoSettings {
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
  String geoipURL;
  int8_t locationUpdateInterval;
  String wifiApSSID;
  String wifiSSID;
  String wifiPwd;
} lilygoSettings;

lilygoSettings setSetting();

#endif
