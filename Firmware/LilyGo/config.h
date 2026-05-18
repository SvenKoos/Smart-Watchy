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

#endif
