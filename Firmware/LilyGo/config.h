#ifndef CONFIG_H
#define CONFIG_H

// wifi
#define WIFI_AP_TIMEOUT 60
#define WIFI_AP_SSID    "LilyGo"

// UI state
#define DARK_STATE 0
#define WATCHFACE_STATE 1
#define MENU_STATE 2
#define ALERT_STATE 3

// error codes
#define CODE_NO_ERROR	0
#define CODE_COMM_ERROR	1
#define CODE_HTTP_ERROR 2
#define CODE_DATA_ERROR 3
#define CODE_PARSE_ERROR 4

// UI timeouts
#define BRIGHTNESS_TIMEOUT_MENU 15
#define BRIGHTNESS_TIMEOUT_DEFAULT 10
#define BRIGHTNESS_TIMEOUT_ALERT 10

// UI themes
#define THEME_ALERT_DATA 1
#define THEME_POWER_DATA 2
#define THEME_ACCELL_DATA 3
#define THEME_LOCATION_DATA 4
#define THEME_WEATHER_DATA 5
#define THEME_DATE_DATA 6
#define THEME_TIME_DATA 7
#define THEME_MENU 8

// watch types
#define DIGITAL_WATCH 1
#define ANALOGUE_WATCH 2
#define QLOCKTWO_WATCH 3

// encryption
// 16-Byte Schlüssel (128-Bit), den T-Watch und T-Echo teilen
const byte key[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

#endif
