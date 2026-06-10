#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "watchFace.h"
#include "dataCollection.h"
#include "icons.h"
#include "accellData.h"
#include "alertData.h"
#include "powerData.h"
#include "locationData.h"
#include "weatherData.h"
#include "config.h"
#include "settings.h"

extern accellData currentAccelleration;
extern alertData currentAlerts;
extern powerData currentPower;
extern locationData currentLocation;
extern weatherData currentWeather;

extern bool WIFI_CONFIGURED;
extern bool WIFI_CONNECTED;

extern lilygoSettings settings;

extern lv_color_t color_bg;
extern lv_color_t color_text;

static lv_display_t *display;
static lv_obj_t *screen;
static lv_style_t styleSmall;
static lv_style_t styleMedium;
static lv_style_t styleLarge;

void watchFaceSetup() {
  // styles
  // Set to built-in SMALL
  lv_style_init(&styleSmall);
  lv_style_set_text_font(&styleSmall, &lv_font_montserrat_24);
  lv_style_set_border_width(&styleSmall, 0);
  // Set to built-in MEDIUM
  lv_style_init(&styleMedium);
  lv_style_set_text_font(&styleMedium, &lv_font_montserrat_36);
  lv_style_set_border_width(&styleMedium, 0);
  // Set to built-in LARGE
  lv_style_init(&styleLarge);
  lv_style_set_text_font(&styleLarge, &lv_font_montserrat_48);
  lv_style_set_border_width(&styleLarge, 0);

  // weather
  LV_IMAGE_DECLARE(map01d);
  LV_IMAGE_DECLARE(map01n);
  LV_IMAGE_DECLARE(map02d);
  LV_IMAGE_DECLARE(map02n);
  LV_IMAGE_DECLARE(map03d);
  LV_IMAGE_DECLARE(map03n);
  LV_IMAGE_DECLARE(map04d);
  LV_IMAGE_DECLARE(map04n);
  LV_IMAGE_DECLARE(map09d);
  LV_IMAGE_DECLARE(map09n);
  LV_IMAGE_DECLARE(map10d);
  LV_IMAGE_DECLARE(map10n);
  LV_IMAGE_DECLARE(map11d);
  LV_IMAGE_DECLARE(map11n);
  LV_IMAGE_DECLARE(map13d);
  LV_IMAGE_DECLARE(map13n);
  LV_IMAGE_DECLARE(map50d);
  LV_IMAGE_DECLARE(map50n);
}

void drawWatchFace() {
  Serial.println("drawWatchFace Start");

  // display
  // 1. Get the current display
  display = lv_display_get_default();

  // screen
  // get active screen
  screen = lv_screen_active();
  // Remove borders
  lv_obj_set_style_border_width(screen, 0, LV_PART_MAIN);
  // Den aktuell aktiven Bildschirm weiß färben
  lv_obj_set_style_bg_color(screen, color_bg, 0);
  // Sicherstellen, dass die Deckkraft auf 100% steht
  lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);
  // Scrollbars abschalten
  lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);
  // Scrolling abschalten
  lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
  // Inhalt des Screens löschen
  lv_obj_clean(screen);

  drawTime();

  drawDate();

  if (currentAccelleration.isMoved) {
    drawSteps();

    drawIcons(WIFI_CONNECTED);

    if (WIFI_CONNECTED) {
      drawWeather();
    }
  }

  // draw the screen
  lv_scr_load(screen);
}

void drawTime() {
  // Create a label on the active screen
  lv_obj_t *label = lv_label_create(screen);

  // Assign the style to the label
  lv_obj_add_style(label, &styleLarge, LV_PART_MAIN);

  // position the label
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 5, 5);

  // set text color according to schema
  lv_obj_set_style_text_color(label, color_text, 0);

  // Set the label text
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[6];
    snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);

    lv_label_set_text(label, buf);
  } else
    lv_label_set_text(label, "Error");
}

void drawDate() {
  const char *month_names[] = {
    "Jan", "Feb", "Mar", "Apr",
    "May", "Jun", "Jul", "Aug",
    "Sep", "Oct", "Nov", "Dec"
  };

  const char *weekday_names[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
  };

  lv_obj_t *labelDay = lv_label_create(screen);
  lv_obj_t *labelMonth = lv_label_create(screen);
  lv_obj_t *labelDayWeek = lv_label_create(screen);

  lv_obj_add_style(labelDay, &styleMedium, LV_PART_MAIN);
  lv_obj_add_style(labelMonth, &styleSmall, LV_PART_MAIN);
  lv_obj_add_style(labelDayWeek, &styleSmall, LV_PART_MAIN);

  lv_obj_set_style_text_color(labelDay, color_text, 0);
  lv_obj_set_style_text_color(labelMonth, color_text, 0);
  lv_obj_set_style_text_color(labelDayWeek, color_text, 0);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    char buf[4];
    int day = timeinfo.tm_mday;  // 1–31
    snprintf(buf, sizeof(buf), "%d", day);
    lv_label_set_text(labelDay, buf);

    int month = timeinfo.tm_mon;  // 0–11
    lv_label_set_text(labelMonth, month_names[month]);

    int wday = timeinfo.tm_wday;  // 0–6
    lv_label_set_text(labelDayWeek, weekday_names[wday]);

    lv_obj_align(labelDay, LV_ALIGN_TOP_RIGHT, -185, 98);
    lv_obj_align(labelMonth, LV_ALIGN_TOP_LEFT, 60, 100);
    lv_obj_align(labelDayWeek, LV_ALIGN_TOP_LEFT, 5, 70);
  }
}

void drawSteps() {
  lv_obj_t *img = lv_image_create(screen);
  lv_image_set_src(img, &iconSteps);
  updateIconTheme(img, settings.displayBGrndBlack);
  lv_obj_align(img, LV_ALIGN_TOP_LEFT, 5, 197);

  lv_obj_t *labelSteps = lv_label_create(screen);
  lv_obj_add_style(labelSteps, &styleMedium, LV_PART_MAIN);
  lv_obj_set_style_text_color(labelSteps, color_text, 0);

  char buf[7];
  snprintf(buf, sizeof(buf), "%d", currentAccelleration.stepCounter);
  lv_label_set_text(labelSteps, buf);
  lv_obj_align(labelSteps, LV_ALIGN_TOP_LEFT, 35, 190);
}

void drawIcons(bool isConnected) {
  if (currentAlerts.count > 0) {
    lv_obj_t *imgAlerts = lv_image_create(screen);
    lv_image_set_src(imgAlerts, &iconNotify);
    updateIconTheme(imgAlerts, settings.displayBGrndBlack);
    lv_obj_align(imgAlerts, LV_ALIGN_TOP_LEFT, 165, 80);
  }

  if (isConnected) {
    lv_obj_t *imgWifi = lv_image_create(screen);
    lv_image_set_src(imgWifi, &iconWifi);
    updateIconTheme(imgWifi, settings.displayBGrndBlack);
    lv_obj_align(imgWifi, LV_ALIGN_TOP_LEFT, 195, 80);
  }

  lv_obj_t *imgBattery = lv_image_create(screen);
  if (currentPower.batteryPercent > 70) {
    lv_image_set_src(imgBattery, &iconBattery);
  } else if (currentPower.batteryPercent > 20) {
    lv_image_set_src(imgBattery, &iconBatteryHalf);
  } else {
    lv_image_set_src(imgBattery, &iconBatteryEmpty);
  }
  updateIconTheme(imgBattery, settings.displayBGrndBlack);
  lv_obj_align(imgBattery, LV_ALIGN_TOP_LEFT, 200, 10);

  lv_obj_t *labelBatteryPercentage = lv_label_create(screen);
  lv_obj_add_style(labelBatteryPercentage, &styleSmall, LV_PART_MAIN);
  lv_obj_set_style_text_align(labelBatteryPercentage, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_set_style_text_color(labelBatteryPercentage, color_text, 0);

  char buf[4];
  snprintf(buf, sizeof(buf), "%d", currentPower.batteryPercent);
  lv_label_set_text(labelBatteryPercentage, buf);
  lv_obj_align(labelBatteryPercentage, LV_ALIGN_TOP_RIGHT, -15, 30);
}

void drawWeather() {
  // location
  lv_obj_t *labelLocation = lv_label_create(screen);
  lv_obj_add_style(labelLocation, &styleSmall, LV_PART_MAIN);
  lv_obj_align(labelLocation, LV_ALIGN_TOP_LEFT, 5, 150);
  lv_obj_set_style_text_color(labelLocation, color_text, 0);
  lv_label_set_text(labelLocation, currentLocation.cityShort);

  if (currentWeather.code == CODE_NO_ERROR) {
    lv_obj_t *imgWeather = lv_image_create(screen);

    if (!settings.displayBGrndBlack) {
      if (strcmp(currentWeather.weatherIcon, "01d") == 0)
        lv_image_set_src(imgWeather, &map01d_white);
      else if (strcmp(currentWeather.weatherIcon, "01n") == 0)
        lv_image_set_src(imgWeather, &map01n_white);
      else if (strcmp(currentWeather.weatherIcon, "02d") == 0)
        lv_image_set_src(imgWeather, &map02d_white);
      else if (strcmp(currentWeather.weatherIcon, "02n") == 0)
        lv_image_set_src(imgWeather, &map02n_white);
      else if (strcmp(currentWeather.weatherIcon, "03d") == 0)
        lv_image_set_src(imgWeather, &map03d_white);
      else if (strcmp(currentWeather.weatherIcon, "03n") == 0)
        lv_image_set_src(imgWeather, &map03n_white);
      else if (strcmp(currentWeather.weatherIcon, "04d") == 0)
        lv_image_set_src(imgWeather, &map04d_white);
      else if (strcmp(currentWeather.weatherIcon, "04n") == 0)
        lv_image_set_src(imgWeather, &map04n_white);
      else if (strcmp(currentWeather.weatherIcon, "09d") == 0)
        lv_image_set_src(imgWeather, &map09d_white);
      else if (strcmp(currentWeather.weatherIcon, "09n") == 0)
        lv_image_set_src(imgWeather, &map09n_white);
      else if (strcmp(currentWeather.weatherIcon, "10d") == 0)
        lv_image_set_src(imgWeather, &map10d_white);
      else if (strcmp(currentWeather.weatherIcon, "10n") == 0)
        lv_image_set_src(imgWeather, &map10n_white);
      else if (strcmp(currentWeather.weatherIcon, "11d") == 0)
        lv_image_set_src(imgWeather, &map11d_white);
      else if (strcmp(currentWeather.weatherIcon, "11n") == 0)
        lv_image_set_src(imgWeather, &map11n_white);
      else if (strcmp(currentWeather.weatherIcon, "13d") == 0)
        lv_image_set_src(imgWeather, &map13d_white);
      else if (strcmp(currentWeather.weatherIcon, "13n") == 0)
        lv_image_set_src(imgWeather, &map13n_white);
      else if (strcmp(currentWeather.weatherIcon, "50d") == 0)
        lv_image_set_src(imgWeather, &map50d_white);
      else if (strcmp(currentWeather.weatherIcon, "50n") == 0)
        lv_image_set_src(imgWeather, &map50n_white);
    } else
    {
      if (strcmp(currentWeather.weatherIcon, "01d") == 0)
        lv_image_set_src(imgWeather, &map01d_black);
      else if (strcmp(currentWeather.weatherIcon, "01n") == 0)
        lv_image_set_src(imgWeather, &map01n_black);
      else if (strcmp(currentWeather.weatherIcon, "02d") == 0)
        lv_image_set_src(imgWeather, &map02d_black);
      else if (strcmp(currentWeather.weatherIcon, "02n") == 0)
        lv_image_set_src(imgWeather, &map02n_black);
      else if (strcmp(currentWeather.weatherIcon, "03d") == 0)
        lv_image_set_src(imgWeather, &map03d_black);
      else if (strcmp(currentWeather.weatherIcon, "03n") == 0)
        lv_image_set_src(imgWeather, &map03n_black);
      else if (strcmp(currentWeather.weatherIcon, "04d") == 0)
        lv_image_set_src(imgWeather, &map04d_black);
      else if (strcmp(currentWeather.weatherIcon, "04n") == 0)
        lv_image_set_src(imgWeather, &map04n_black);
      else if (strcmp(currentWeather.weatherIcon, "09d") == 0)
        lv_image_set_src(imgWeather, &map09d_black);
      else if (strcmp(currentWeather.weatherIcon, "09n") == 0)
        lv_image_set_src(imgWeather, &map09n_black);
      else if (strcmp(currentWeather.weatherIcon, "10d") == 0)
        lv_image_set_src(imgWeather, &map10d_black);
      else if (strcmp(currentWeather.weatherIcon, "10n") == 0)
        lv_image_set_src(imgWeather, &map10n_black);
      else if (strcmp(currentWeather.weatherIcon, "11d") == 0)
        lv_image_set_src(imgWeather, &map11d_black);
      else if (strcmp(currentWeather.weatherIcon, "11n") == 0)
        lv_image_set_src(imgWeather, &map11n_black);
      else if (strcmp(currentWeather.weatherIcon, "13d") == 0)
        lv_image_set_src(imgWeather, &map13d_black);
      else if (strcmp(currentWeather.weatherIcon, "13n") == 0)
        lv_image_set_src(imgWeather, &map13n_black);
      else if (strcmp(currentWeather.weatherIcon, "50d") == 0)
        lv_image_set_src(imgWeather, &map50d_black);
      else if (strcmp(currentWeather.weatherIcon, "50n") == 0)
        lv_image_set_src(imgWeather, &map50n_black);
    }

    lv_image_set_scale(imgWeather, 200);
    lv_obj_align(imgWeather, LV_ALIGN_TOP_LEFT, 140, 155);

    // temperature
    lv_obj_t *labelTemperature = lv_label_create(screen);
    lv_obj_add_style(labelTemperature, &styleLarge, LV_PART_MAIN);
    lv_obj_align(labelTemperature, LV_ALIGN_TOP_RIGHT, -40, 130);
    lv_obj_set_style_text_color(labelTemperature, color_text, 0);
    char buf[3];
    snprintf(buf, sizeof(buf), "%d", currentWeather.temperature);
    lv_label_set_text(labelTemperature, buf);

    // unit
    lv_obj_t *labelUnit = lv_label_create(screen);
    lv_obj_add_style(labelUnit, &styleSmall, LV_PART_MAIN);
    lv_obj_align(labelUnit, LV_ALIGN_TOP_LEFT, 200, 135);
    lv_obj_set_style_text_color(labelUnit, color_text, 0);
    if (currentWeather.isMetric)
      lv_label_set_text(labelUnit, "°C");
    else
      lv_label_set_text(labelUnit, "°F");
  }
}
