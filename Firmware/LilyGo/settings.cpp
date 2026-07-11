#include <lvgl.h>
#include <LilyGoLib.h>

#include "settings.h"
#include "config.h"

extern lv_color_t color_bg;
extern lv_color_t color_text;

lv_color_t colorAlertData;
lv_color_t colorPowerData;
lv_color_t colorAccellData;
lv_color_t colorLocationData;
lv_color_t colorWeatherData;
lv_color_t colorDateData;
lv_color_t colorTimeData;

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
  settings.stepGoal = STEP_GOAL;
  settings.googleApiKey = GOOGLE_APIKEY;
  settings.googleGeoLocationURL = GOOGLE_LOCATION_URL;

  settings.displayBGrndBlack = BACKGROUND_BLACK;
  setTheme(settings.displayBGrndBlack);

  return settings;
}

void setTheme(bool inverted) {
  if (inverted) {  // inverted = white on black
    color_bg = lv_color_black();
    color_text = lv_color_white();
  } else {  // not inverted = black on white
    color_bg = lv_color_white();
    color_text = lv_color_black();
  }

  // LV_PALETTE_RED, LV_PALETTE_PINK, LV_PALETTE_PURPLE, LV_PALETTE_DEEP_PURPLE, LV_PALETTE_INDIGO, LV_PALETTE_BLUE, LV_PALETTE_LIGHT_BLUE
  // LV_PALETTE_CYAN, LV_PALETTE_TEAL, LV_PALETTE_GREEN, LV_PALETTE_LIGHT_GREEN, LV_PALETTE_LIME, LV_PALETTE_YELLOW, LV_PALETTE_AMBER
  // LV_PALETTE_ORANGE, LV_PALETTE_DEEP_ORANGE, LV_PALETTE_BROWN, LV_PALETTE_BLUE_GREY, LV_PALETTE_GREY
  colorAlertData = lv_palette_main(LV_PALETTE_LIGHT_BLUE);
  colorPowerData = lv_palette_main(LV_PALETTE_GREY);
  colorAccellData = lv_palette_main(LV_PALETTE_GREY);
  colorLocationData = lv_palette_main(LV_PALETTE_PINK);
  colorWeatherData = lv_palette_main(LV_PALETTE_PINK);
  colorDateData = lv_palette_main(LV_PALETTE_DEEP_ORANGE);
  colorTimeData = color_text;
}

// Funktion, die deine Icons je nach Modus anpasst
void updateIconTheme(lv_obj_t* icon_obj, bool inverted) {
  if (inverted) {
    // Schalte das Icon auf WEISS um (Invertiert auf schwarzem Grund)
    // lv_obj_set_style_image_recolor(icon_obj, lv_color_white(), 0);
    lv_obj_set_style_image_recolor(icon_obj, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_image_recolor_opa(icon_obj, LV_OPA_COVER, 0);  // 100% Deckkraft
  } else {
    // Schalte das Icon auf SCHWARZ um (Standard auf weissem Grund)
    // lv_obj_set_style_image_recolor(icon_obj, lv_color_black(), 0);
    lv_obj_set_style_image_recolor(icon_obj, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_image_recolor_opa(icon_obj, LV_OPA_COVER, 0); // 100% Deckkraft

    // Alternativ, falls Schwarz die Originalfarbe des Bildes ist:
    // lv_obj_set_style_image_recolor_opa(icon_obj, LV_OPA_TRANSP, 0); // Schaltet Einfärbung ab
  }
}

lv_color_t GetTheme(int themeID) {
  lv_color_t color;

  color = color_text;
  switch (themeID) {
    case THEME_ALERT_DATA: color = colorAlertData; break;
    case THEME_POWER_DATA: color = colorPowerData; break;
    case THEME_ACCELL_DATA: color = colorAccellData; break;
    case THEME_LOCATION_DATA: color = colorLocationData; break;
    case THEME_WEATHER_DATA: color = colorWeatherData; break;
    case THEME_DATE_DATA: color = colorDateData; break;
    case THEME_TIME_DATA: color = colorTimeData; break;
  }

  return color;
}
