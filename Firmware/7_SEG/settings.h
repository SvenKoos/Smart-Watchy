#ifndef SETTINGS_H
#define SETTINGS_H

//Weather Settings
//#define CITY_ID "5128581" //New York City https://openweathermap.org/current#cityid
#define CITY_ID "2660253" //Interlaken
#define OPENWEATHERMAP_APIKEY "5bcd8b064ea29fe07547ac723e4a2875" 
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather" //open weather api
#define TEMP_UNIT "metric" //metric = Celsius , imperial = Fahrenheit
#define TEMP_LANG "en"
#define WEATHER_UPDATE_INTERVAL 15 //default 30, must be greater than 5, measured in minutes
//NTP Settings
#define NTP_SERVER "pool.ntp.org"
// #define GMT_OFFSET_SEC 3600 * -5 //New York is UTC -5 EST, -4 EDT
#define GMT_OFFSET_SEC 3600 * 1 //CET
// SvKo: added
#define GEOIP_URL "http://ipwho.is"  //  GeoIP (Get IP address location in JSON format)
#define LOCATION_UPDATE_INTERVAL 15

watchySettings settings{
    CITY_ID,
    OPENWEATHERMAP_APIKEY,
    OPENWEATHERMAP_URL,
    TEMP_UNIT,
    TEMP_LANG,
    WEATHER_UPDATE_INTERVAL,
    NTP_SERVER,
    GMT_OFFSET_SEC,
    // SvKo added
    0,
    GEOIP_URL,
    LOCATION_UPDATE_INTERVAL
};

#endif