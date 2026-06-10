#include <lvgl.h>
#include <LilyGoLib.h>

#include "settings.h"

extern lv_color_t color_bg;
extern lv_color_t color_text;

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

  settings.displayBGrndBlack = true;
  setTheme(settings.displayBGrndBlack);

  return settings;
}

void setTheme(bool inverted) {
    if (inverted) {     // inverted = white on black
        color_bg   = lv_color_black();
        color_text = lv_color_white();
    } else {            // not inverted = black on white
        color_bg   = lv_color_white();
        color_text = lv_color_black();
    }
}

// Funktion, die deine Icons je nach Modus anpasst
void updateIconTheme(lv_obj_t * icon_obj, bool inverted) {
    if (inverted) {
        // Schalte das Icon auf WEISS um (Invertiert auf schwarzem Grund)
        lv_obj_set_style_image_recolor(icon_obj, lv_color_white(), 0);
        lv_obj_set_style_image_recolor_opa(icon_obj, LV_OPA_COVER, 0); // 100% Deckkraft
    } else {
        // Schalte das Icon auf SCHWARZ um (Standard auf weissem Grund)
        // lv_obj_set_style_image_recolor(icon_obj, lv_color_black(), 0);
        // lv_obj_set_style_image_recolor_opa(icon_obj, LV_OPA_COVER, 0); // 100% Deckkraft
        
        // Alternativ, falls Schwarz die Originalfarbe des Bildes ist:
        // lv_obj_set_style_image_recolor_opa(icon_obj, LV_OPA_TRANSP, 0); // Schaltet Einfärbung ab
    }
}
