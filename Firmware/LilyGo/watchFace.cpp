#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>
#include <cmath>

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
extern bool newAlertsIndicator;

extern bool WIFI_CONFIGURED;
extern bool WIFI_CONNECTED;

extern lilygoSettings settings;

extern lv_color_t color_bg;
extern lv_color_t color_text;

extern int watchType;

static lv_display_t *display;
static lv_obj_t *screen;
static lv_style_t styleMicro;
static lv_style_t styleSmall;
static lv_style_t styleMedium;
static lv_style_t styleLarge;

LV_FONT_DECLARE(emoji);
static lv_font_t watchface_font;

void watchFaceSetup() {
  // styles
  // Set to built-in MICRO
  lv_style_init(&styleMicro);
  lv_style_set_text_font(&styleMicro, &lv_font_montserrat_18);
  lv_style_set_border_width(&styleMicro, 0);
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

  if (currentAccelleration.isMoved) {
    drawBattery();

    if (WIFI_CONNECTED) {
      drawWeather();

      drawSolarArc(currentWeather.currentSunrise, currentWeather.currentSunset, currentWeather.currentDT);

      if (newAlertsIndicator == true) {
        drawAlert();
      }
    }

    drawSteps();
  }

  if (watchType == DIGITAL_WATCH) {
    // digital clock
    drawDate();
    drawTime();
  } else if (watchType == ANALOGUE_WATCH) {
    // analogue clock
    drawAnalogClock();
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
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 5);

  // set text color according to schema
  lv_obj_set_style_text_color(label, GetTheme(THEME_TIME_DATA), 0);

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

  lv_obj_set_style_text_color(labelDay, GetTheme(THEME_DATE_DATA), 0);
  lv_obj_set_style_text_color(labelMonth, GetTheme(THEME_DATE_DATA), 0);
  lv_obj_set_style_text_color(labelDayWeek, GetTheme(THEME_DATE_DATA), 0);

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

    lv_obj_align(labelDay, LV_ALIGN_TOP_RIGHT, -180, 98);
    lv_obj_align(labelMonth, LV_ALIGN_TOP_LEFT, 65, 100);
    lv_obj_align(labelDayWeek, LV_ALIGN_TOP_LEFT, 10, 70);
  }
}

void drawSteps() {
  lv_obj_t *labelSymbol = lv_label_create(screen);
  lv_obj_add_style(labelSymbol, &styleMedium, 0);
  lv_label_set_text(labelSymbol, LV_SYMBOL_IMAGE "");
  lv_obj_align(labelSymbol, LV_ALIGN_TOP_LEFT, 15, 192);
  lv_obj_set_style_text_color(labelSymbol, GetTheme(THEME_ACCELL_DATA), 0);

  // 2. Schrittzahl-Label (Jetzt in styleSmall für mehr Platz)
  lv_obj_t *labelSteps = lv_label_create(screen);
  lv_obj_add_style(labelSteps, &styleSmall, LV_PART_MAIN);  // Geändert auf styleSmall
  lv_obj_set_style_text_color(labelSteps, GetTheme(THEME_ACCELL_DATA), 0);

  char buf[7];
  snprintf(buf, sizeof(buf), "%d", currentAccelleration.stepCounter);
  lv_label_set_text(labelSteps, buf);
  // Leicht nach oben gezogen (Y=190), damit der Balken darunter passt
  lv_obj_align(labelSteps, LV_ALIGN_TOP_LEFT, 55, 192);

  // 3. Der Fortschrittsbalken (Bar)
  lv_obj_t *barSteps = lv_bar_create(screen);

  // Breite anpassen (z.B. 100 Pixel lang, 4 Pixel hoch für einen filigranen Look)
  lv_obj_set_size(barSteps, 100, 4);

  // Bereich von 0 bis 10.000 Schritten definieren
  lv_bar_set_range(barSteps, 0, settings.stepGoal);
  lv_bar_set_value(barSteps, currentAccelleration.stepCounter, LV_ANIM_OFF);

  // STYLING FÜR DEN ERREICHTEN TEIL (INDIKATOR):
  // Setzt deine Theme-Farbe (hellgrau) und erzwingt die volle Deckkraft
  // lv_obj_set_style_bg_color(barSteps, GetTheme(THEME_ACCELL_DATA), LV_PART_INDICATOR);
  // lv_obj_set_style_bg_opa(barSteps, LV_OPA_COVER, LV_PART_INDICATOR); // Verhindert das Standard-Blau

  // STYLING FÜR DEN UNERREICHTEN TEIL (MAIN):
  // Exakt das gleiche Dunkelgrau wie beim Batterie-Ring
  lv_obj_set_style_bg_color(barSteps, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(barSteps, LV_OPA_COVER, LV_PART_MAIN);

  // Positionierung
  lv_obj_align_to(barSteps, labelSteps, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 5);
}

void drawBattery() {
  // 1. Die dynamische Farbe anhand der Prozent ermitteln
  lv_color_t batteryColor;
  if (currentPower.batteryPercent > 70) {
    batteryColor = lv_palette_main(LV_PALETTE_GREEN);
  } else if (currentPower.batteryPercent > 20) {
    batteryColor = lv_palette_main(LV_PALETTE_ORANGE);
  } else {
    batteryColor = lv_palette_main(LV_PALETTE_RED);
  }

  // 2. Den Batterie-Ring (Arc) erstellen
  lv_obj_t *arcBattery = lv_arc_create(screen);

  // Größe so wählen, dass er die Zahl elegant umschließt (z.B. 45x45 Pixel)
  lv_obj_set_size(arcBattery, 45, 45);

  // Start bei 12 Uhr (270 Grad) und Ende je nach Prozent im Uhrzeigersinn
  lv_arc_set_rotation(arcBattery, 270);
  lv_arc_set_bg_angles(arcBattery, 0, 360);  // Der graue Hintergrund-Ring ist geschlossen
  lv_arc_set_value(arcBattery, currentPower.batteryPercent);

  // Den interaktiven "Knopf" des Arcs verstecken, wir wollen nur den Ring sehen
  lv_obj_remove_style(arcBattery, NULL, LV_PART_KNOB);
  lv_obj_remove_flag(arcBattery, LV_OBJ_FLAG_CLICKABLE);  // Keine Touch-Interaktion

  // Styling für den aktiven (Vordergrund) und inaktiven (Hintergrund) Ring
  lv_obj_set_style_arc_color(arcBattery, batteryColor, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arcBattery, 3, LV_PART_INDICATOR);  // 3 Pixel dünner Ring

  // Hintergrundring dezent dunkelgrau oder leicht transparent halten
  lv_obj_set_style_arc_color(arcBattery, lv_palette_darken(LV_PALETTE_GREY, 3), LV_PART_MAIN);
  lv_obj_set_style_arc_width(arcBattery, 3, LV_PART_MAIN);

  // Positionierung oben rechts (da wo vorher das Icon-Areal war)
  lv_obj_align(arcBattery, LV_ALIGN_TOP_RIGHT, -12, 12);

  // 3. Die Prozentzahl exakt im Ring zentrieren
  lv_obj_t *labelBatteryPercentage = lv_label_create(screen);
  lv_obj_add_style(labelBatteryPercentage, &styleMicro, LV_PART_MAIN);

  lv_obj_set_style_text_color(labelBatteryPercentage, GetTheme(THEME_POWER_DATA), 0);

  char buf[4];
  snprintf(buf, sizeof(buf), "%d", currentPower.batteryPercent);
  lv_label_set_text(labelBatteryPercentage, buf);

  // LV_ALIGN_CENTER direkt auf den arcBattery beziehen, damit es mathematisch perfekt mittig sitzt!
  lv_obj_align_to(labelBatteryPercentage, arcBattery, LV_ALIGN_CENTER, 0, 0);
}

void drawWeather() {
  // Einheitliche Farbe für alle Wetter-Elemente holen
  lv_color_t weatherColor = GetTheme(THEME_WEATHER_DATA);

  // 1. LOCATION
  lv_obj_t *labelLocation = lv_label_create(screen);
  if (strlen(currentLocation.cityShort) < 8)
    lv_obj_add_style(labelLocation, &styleSmall, LV_PART_MAIN);
  else
    lv_obj_add_style(labelLocation, &styleMicro, LV_PART_MAIN);
  // Fester Startpunkt links im unteren Drittel
  lv_obj_align(labelLocation, LV_ALIGN_TOP_LEFT, 10, 155);
  lv_obj_set_style_text_color(labelLocation, weatherColor, 0);  // Einheitliche Farbe
  lv_label_set_text(labelLocation, currentLocation.cityShort);

  if (currentWeather.code == CODE_NO_ERROR) {
    // 2. WEATHER ICON
    lv_obj_t *imgWeather = lv_image_create(screen);

    if (!settings.displayBGrndBlack) {
      if (strcmp(currentWeather.weatherIcon, "01d") == 0) lv_image_set_src(imgWeather, &map01d_white);
      else if (strcmp(currentWeather.weatherIcon, "01n") == 0) lv_image_set_src(imgWeather, &map01n_white);
      else if (strcmp(currentWeather.weatherIcon, "02d") == 0) lv_image_set_src(imgWeather, &map02d_white);
      else if (strcmp(currentWeather.weatherIcon, "02n") == 0) lv_image_set_src(imgWeather, &map02n_white);
      else if (strcmp(currentWeather.weatherIcon, "03d") == 0) lv_image_set_src(imgWeather, &map03d_white);
      else if (strcmp(currentWeather.weatherIcon, "03n") == 0) lv_image_set_src(imgWeather, &map03n_white);
      else if (strcmp(currentWeather.weatherIcon, "04d") == 0) lv_image_set_src(imgWeather, &map04d_white);
      else if (strcmp(currentWeather.weatherIcon, "04n") == 0) lv_image_set_src(imgWeather, &map04n_white);
      else if (strcmp(currentWeather.weatherIcon, "09d") == 0) lv_image_set_src(imgWeather, &map09d_white);
      else if (strcmp(currentWeather.weatherIcon, "09n") == 0) lv_image_set_src(imgWeather, &map09n_white);
      else if (strcmp(currentWeather.weatherIcon, "10d") == 0) lv_image_set_src(imgWeather, &map10d_white);
      else if (strcmp(currentWeather.weatherIcon, "10n") == 0) lv_image_set_src(imgWeather, &map10n_white);
      else if (strcmp(currentWeather.weatherIcon, "11d") == 0) lv_image_set_src(imgWeather, &map11d_white);
      else if (strcmp(currentWeather.weatherIcon, "11n") == 0) lv_image_set_src(imgWeather, &map11n_white);
      else if (strcmp(currentWeather.weatherIcon, "13d") == 0) lv_image_set_src(imgWeather, &map13d_white);
      else if (strcmp(currentWeather.weatherIcon, "13n") == 0) lv_image_set_src(imgWeather, &map13n_white);
      else if (strcmp(currentWeather.weatherIcon, "50d") == 0) lv_image_set_src(imgWeather, &map50d_white);
      else if (strcmp(currentWeather.weatherIcon, "50n") == 0) lv_image_set_src(imgWeather, &map50n_white);
    } else {
      if (strcmp(currentWeather.weatherIcon, "01d") == 0) lv_image_set_src(imgWeather, &map01d_black);
      else if (strcmp(currentWeather.weatherIcon, "01n") == 0) lv_image_set_src(imgWeather, &map01n_black);
      else if (strcmp(currentWeather.weatherIcon, "02d") == 0) lv_image_set_src(imgWeather, &map02d_black);
      else if (strcmp(currentWeather.weatherIcon, "02n") == 0) lv_image_set_src(imgWeather, &map02n_black);
      else if (strcmp(currentWeather.weatherIcon, "03d") == 0) lv_image_set_src(imgWeather, &map03d_black);
      else if (strcmp(currentWeather.weatherIcon, "03n") == 0) lv_image_set_src(imgWeather, &map03n_black);
      else if (strcmp(currentWeather.weatherIcon, "04d") == 0) lv_image_set_src(imgWeather, &map04d_black);
      else if (strcmp(currentWeather.weatherIcon, "04n") == 0) lv_image_set_src(imgWeather, &map04n_black);
      else if (strcmp(currentWeather.weatherIcon, "09d") == 0) lv_image_set_src(imgWeather, &map09d_black);
      else if (strcmp(currentWeather.weatherIcon, "09n") == 0) lv_image_set_src(imgWeather, &map09n_black);
      else if (strcmp(currentWeather.weatherIcon, "10d") == 0) lv_image_set_src(imgWeather, &map10d_black);
      else if (strcmp(currentWeather.weatherIcon, "10n") == 0) lv_image_set_src(imgWeather, &map10n_black);
      else if (strcmp(currentWeather.weatherIcon, "11d") == 0) lv_image_set_src(imgWeather, &map11d_black);
      else if (strcmp(currentWeather.weatherIcon, "11n") == 0) lv_image_set_src(imgWeather, &map11n_black);
      else if (strcmp(currentWeather.weatherIcon, "13d") == 0) lv_image_set_src(imgWeather, &map13d_black);
      else if (strcmp(currentWeather.weatherIcon, "13n") == 0) lv_image_set_src(imgWeather, &map13n_black);
      else if (strcmp(currentWeather.weatherIcon, "50d") == 0) lv_image_set_src(imgWeather, &map50d_black);
      else if (strcmp(currentWeather.weatherIcon, "50n") == 0) lv_image_set_src(imgWeather, &map50n_black);
    }

    lv_image_set_scale(imgWeather, 200);
    // DYNAMISCH: Das Icon wird direkt rechts neben das Location-Label gekettet
    lv_obj_align_to(imgWeather, labelLocation, LV_ALIGN_OUT_RIGHT_MID, -10, 0);

    // 3. TEMPERATURE
    lv_obj_t *labelTemperature = lv_label_create(screen);
    lv_obj_add_style(labelTemperature, &styleSmall, LV_PART_MAIN);  // Medium statt Large für eine harmonische Zeile
    lv_obj_set_style_text_color(labelTemperature, weatherColor, 0);

    char buf[6];  // Puffer leicht vergrößert für Sicherheit
    snprintf(buf, sizeof(buf), "%d", currentWeather.temperature);
    lv_label_set_text(labelTemperature, buf);

    // DYNAMISCH: Die Temperatur folgt direkt rechts neben dem Icon
    lv_obj_align_to(labelTemperature, imgWeather, LV_ALIGN_OUT_RIGHT_MID, -10, 0);

    // 4. UNIT (°C / °F)
    lv_obj_t *labelUnit = lv_label_create(screen);
    lv_obj_add_style(labelUnit, &styleMicro, LV_PART_MAIN);
    lv_obj_set_style_text_color(labelUnit, weatherColor, 0);

    if (currentWeather.isMetric)
      lv_label_set_text(labelUnit, "°C");
    else
      lv_label_set_text(labelUnit, "°F");

    // DYNAMISCH: Die Einheit klebt direkt oben rechts neben der Temperatur-Zahl
    lv_obj_align_to(labelUnit, labelTemperature, LV_ALIGN_OUT_RIGHT_TOP, 0, -2);
  }
}

void drawSolarArc(uint32_t sunrise_timestamp, uint32_t sunset_timestamp, uint32_t current_timestamp) {
  // 1. Basis-Hintergrundfarben aus deinem System
  lv_color_t arc_bg_color = lv_palette_darken(LV_PALETTE_GREY, 3);  // Dezenter Bogen-Hintergrund (~RGB 60)
  lv_color_t accent_color = GetTheme(THEME_DATE_DATA);              // Deine aktive Themenfarbe (z.B. Orange/Gelb)

  // NEU: Farben für die Kreise aus dem Bogen-Theme abgeleitet
  lv_color_t sunrise_color = lv_palette_lighten(LV_PALETTE_GREY, 2);  // Helleres Grau für Aufgang (~RGB 180)
  lv_color_t sunset_color = lv_palette_darken(LV_PALETTE_GREY, 4);    // Sehr dunkles Anthrazit für Untergang (~RGB 50)

  // 2. Haupt-Container für das Solar-Widget erstellen (Auf 100x70 angepasst)
  lv_obj_t *solar_cont = lv_obj_create(lv_screen_active());
  lv_obj_set_size(solar_cont, 100, 70);
  lv_obj_set_style_bg_opa(solar_cont, LV_OPA_TRANSP, 0);  // Transparent
  lv_obj_set_style_border_width(solar_cont, 0, 0);
  lv_obj_set_style_pad_all(solar_cont, 0, 0);

  // RETTUNG GEGEN DEN GRAUEN STRICH: Schaltet die automatische Scrollbar komplett ab!
  lv_obj_set_scrollbar_mode(solar_cont, LV_SCROLLBAR_MODE_OFF);

  // Positionierung (unverändert)
  lv_obj_align(solar_cont, LV_ALIGN_TOP_LEFT, 135, 60);

  // 3. Der Sonnenbogen (Arc)
  lv_obj_t *arc = lv_arc_create(solar_cont);
  lv_obj_set_size(arc, 80, 80);  // Bogen füllt die Breite

  // RETTUNG: Wir nutzen wieder TOP_MID, drücken den Bogen aber mit +25px deutlich tiefer!
  // Falls er noch einen Tick zu hoch/tief ist, einfach den Wert 25 anpassen.
  lv_obj_align(arc, LV_ALIGN_TOP_MID, 0, 10);

  // Winkel (unverändert ein perfekter oberer Halbkreis)
  lv_arc_set_angles(arc, 180, 360);
  lv_arc_set_bg_angles(arc, 180, 360);

  lv_obj_set_style_arc_width(arc, 4, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, arc_bg_color, LV_PART_MAIN);

  lv_obj_set_style_arc_width(arc, 4, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc, accent_color, LV_PART_INDICATOR);

  lv_obj_set_style_bg_color(arc, lv_color_white(), LV_PART_KNOB);
  lv_obj_set_style_pad_all(arc, 2, LV_PART_KNOB);

  // 4. Berechnung der aktuellen Sonnenposition (0 bis 100%)
  int32_t total_daylight = sunset_timestamp - sunrise_timestamp;
  int32_t current_progress = current_timestamp - sunrise_timestamp;
  int32_t percent = 0;
  bool is_night = false;

  if (total_daylight > 0) {
    percent = (current_progress * 100) / total_daylight;

    // Prüfen, ob wir uns außerhalb der Tageszeit befinden
    if (current_timestamp < sunrise_timestamp || current_timestamp > sunset_timestamp) {
      is_night = true;
    }

    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
  }

  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_value(arc, percent);

  // Dynamische Farbanpassung für die Nacht
  if (is_night) {
    // Nachts: Der aktive Bogen wird unsichtbar/grau wie der Hintergrund
    lv_obj_set_style_arc_color(arc, arc_bg_color, LV_PART_INDICATOR);
    // Nachts: Den weißen Punkt (Sonne) komplett ausblenden
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
  } else {
    // Tagsüber: Normale Akzentfarbe und weiße Sonne
    lv_obj_set_style_arc_color(arc, accent_color, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(arc, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_KNOB);
    lv_obj_set_style_bg_opa(arc, LV_OPA_COVER, LV_PART_KNOB);
  }

  // 5. Linker Kreis: Sonnenaufgang (Nur noch reine Farbe, keine Emojis)
  lv_obj_t *btn_sunrise = lv_obj_create(solar_cont);
  lv_obj_set_size(btn_sunrise, 16, 16);                    // Etwas verkleinert (16x16), wirkt eleganter als reiner Farbpunkt
  lv_obj_align(btn_sunrise, LV_ALIGN_BOTTOM_LEFT, 6, -4);  // Perfekt an den flachen Bogenrand geschmiegt
  lv_obj_set_style_radius(btn_sunrise, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn_sunrise, sunrise_color, 0);  // Die hellere Farbe des Themes
  lv_obj_set_style_bg_opa(btn_sunrise, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(btn_sunrise, 0, 0);  // Keine Border nötig, da vollflächig farbig
  lv_obj_set_style_pad_all(btn_sunrise, 0, 0);

  // 6. Rechter Kreis: Sonnenuntergang (Nur noch reine Farbe)
  lv_obj_t *btn_sunset = lv_obj_create(solar_cont);
  lv_obj_set_size(btn_sunset, 16, 16);
  lv_obj_align(btn_sunset, LV_ALIGN_BOTTOM_RIGHT, -6, -4);
  lv_obj_set_style_radius(btn_sunset, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn_sunset, sunset_color, 0);  // Die dunklere Farbe des Themes
  lv_obj_set_style_bg_opa(btn_sunset, LV_OPA_COVER, 0);
  lv_obj_set_style_border_width(btn_sunset, 0, 0);
  lv_obj_set_style_pad_all(btn_sunset, 0, 0);
}

void drawAlert() {
  // Erstellt ein Objekt über den gesamten Bildschirm
  lv_obj_t *alert_frame = lv_obj_create(lv_screen_active());
  lv_obj_set_size(alert_frame, LV_PCT(100), LV_PCT(100));
  lv_obj_center(alert_frame);

  // Grund-Styles: Absolut transparent im Inneren, keine Paddings
  lv_obj_set_style_bg_opa(alert_frame, LV_OPA_TRANSP, 0);
  lv_obj_set_style_pad_all(alert_frame, 0, 0);

  // Leicht abgerundete Ecken für den Rahmen (z.B. 16px oder je nach Display)
  lv_obj_set_style_radius(alert_frame, 16, 0);

  // Wichtig: Klicks durchlassen, damit das restliche Watchface bedienbar bleibt
  lv_obj_add_flag(alert_frame, LV_OBJ_FLAG_CLICK_FOCUSABLE);
  lv_obj_remove_flag(alert_frame, LV_OBJ_FLAG_SCROLLABLE);

  // Rahmen aktivieren: 3 Pixel breit in der Alarm-Themenfarbe
  lv_obj_set_style_border_color(alert_frame, GetTheme(THEME_ALERT_DATA), 0);
  lv_obj_set_style_border_width(alert_frame, 2, 0);
  lv_obj_set_style_border_opa(alert_frame, LV_OPA_COVER, 0);
}

// Speicher für die Koordinaten der Zeiger (X0, Y0, X1, Y1)
static lv_point_precise_t hour_points[2] = { { 60, 60 }, { 60, 30 } };
static lv_point_precise_t min_points[2] = { { 60, 60 }, { 60, 15 } };

void drawAnalogClock() {
  lv_color_t main_white = lv_color_white();                        // Knackiges Weiß für Ziffern & Hauptstriche
  lv_color_t minor_gray = lv_palette_lighten(LV_PALETTE_GREY, 2);  // Klares Hellgrau für Zwischenstriche
  lv_color_t accent_color = GetTheme(THEME_DATE_DATA);             // Deine Datumsfarbe

  // 1. Haupt-Container vergrößert auf 120x120
  lv_obj_t *clock_cont = lv_obj_create(lv_screen_active());
  lv_obj_set_size(clock_cont, 120, 120);
  lv_obj_align(clock_cont, LV_ALIGN_TOP_LEFT, 12, 12);  // Abstand 10 von links und oben
  lv_obj_set_style_bg_opa(clock_cont, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(clock_cont, 0, 0);
  lv_obj_set_style_pad_all(clock_cont, 0, 0);
  lv_obj_set_scrollbar_mode(clock_cont, LV_SCROLLBAR_MODE_OFF);

  // 2. Das Ziffernblatt (Skala) - Jetzt REIN für die Striche, ohne Text-Stress
  lv_obj_t *scale = lv_scale_create(clock_cont);
  lv_obj_set_size(scale, 120, 120);
  lv_obj_center(scale);

  lv_scale_set_mode(scale, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_rotation(scale, 270);  // 12 Uhr oben
  lv_scale_set_angle_range(scale, 360);

  lv_scale_set_range(scale, 0, 12);
  lv_scale_set_total_tick_count(scale, 13);
  lv_scale_set_major_tick_every(scale, 3);  // Striche bei 12, 3, 6, 9

  // STYLES für Striche
  lv_obj_set_style_length(scale, 8, LV_PART_INDICATOR);  // Schöne, feine Striche
  lv_obj_set_style_length(scale, 4, LV_PART_ITEMS);
  lv_obj_set_style_line_color(scale, main_white, LV_PART_INDICATOR);
  lv_obj_set_style_line_color(scale, minor_gray, LV_PART_ITEMS);
  lv_obj_set_style_line_width(scale, 2, LV_PART_INDICATOR);

  lv_obj_set_style_arc_color(scale, minor_gray, LV_PART_MAIN);
  lv_obj_set_style_arc_width(scale, 1, LV_PART_MAIN);

  lv_obj_set_style_text_opa(scale, LV_OPA_TRANSP, LV_PART_MAIN);

  // ==========================================
  // NEU: Absolute Kontrolle über die 4 Zahlen via Labels
  // ==========================================

  // 12 Uhr (Oben zentriert, leicht nach unten versetzt)
  lv_obj_t *lbl_12 = lv_label_create(clock_cont);
  lv_obj_set_style_text_font(lbl_12, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_12, main_white, 0);
  lv_label_set_text(lbl_12, "12");
  lv_obj_align(lbl_12, LV_ALIGN_TOP_MID, 0, 12);

  // 6 Uhr (Unten zentriert, leicht nach oben versetzt)
  lv_obj_t *lbl_6 = lv_label_create(clock_cont);
  lv_obj_set_style_text_font(lbl_6, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_6, main_white, 0);
  lv_label_set_text(lbl_6, "6");
  lv_obj_align(lbl_6, LV_ALIGN_BOTTOM_MID, 0, -12);

  // 9 Uhr (Links zentriert, leicht nach rechts versetzt)
  lv_obj_t *lbl_9 = lv_label_create(clock_cont);
  lv_obj_set_style_text_font(lbl_9, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_9, main_white, 0);
  lv_label_set_text(lbl_9, "9");
  lv_obj_align(lbl_9, LV_ALIGN_LEFT_MID, 12, 0);

  // 3 Uhr (Rechts zentriert, leicht nach links versetzt)
  lv_obj_t *lbl_3 = lv_label_create(clock_cont);
  lv_obj_set_style_text_font(lbl_3, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(lbl_3, main_white, 0);
  lv_label_set_text(lbl_3, "3");
  lv_obj_align(lbl_3, LV_ALIGN_RIGHT_MID, -12, 0);

  // 3. Datumsanzeige (Perfekt LINKS neben der eben erstellten "3")
  lv_obj_t *date_label = lv_label_create(clock_cont);
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(date_label, accent_color, 0);

  // Da die "3" bei -12px vom rechten Rand sitzt, platzieren wir das Datum
  // einfach bei -26px. So steht es wunderschön links daneben!
  lv_obj_align(date_label, LV_ALIGN_RIGHT_MID, -26, 0);

  // 4. Zeiger (Stunde)
  lv_obj_t *hour_line = lv_line_create(clock_cont);
  lv_line_set_points(hour_line, hour_points, 2);
  lv_obj_set_style_line_width(hour_line, 4, 0);
  lv_obj_set_style_line_color(hour_line, main_white, 0);
  lv_obj_set_style_line_rounded(hour_line, true, 0);

  // 5. Zeiger (Minute)
  lv_obj_t *min_line = lv_line_create(clock_cont);
  lv_line_set_points(min_line, min_points, 2);
  lv_obj_set_style_line_width(min_line, 2, 0);
  lv_obj_set_style_line_color(min_line, minor_gray, 0);
  lv_obj_set_style_line_rounded(min_line, true, 0);

  struct tm timeinfo;
  int hours;
  int minutes;
  int day;

  if (getLocalTime(&timeinfo)) {
    hours = timeinfo.tm_hour;
    minutes = timeinfo.tm_min;
    day = timeinfo.tm_mday;
  }

  lv_label_set_text_fmt(date_label, "%02d", day);

  // NEU: Neuer Mittelpunkt für 120x120
  const int cx = 60;
  const int cy = 60;

  // NEU: Längen an die neue Skala angepasst (Zahlen liegen weiter innen)
  const int hour_len = 28;
  const int min_len = 42;

  double min_angle = (minutes * 6.0) * M_PI / 180.0 - M_PI_2;
  double hour_angle = ((hours % 12) * 30.0 + minutes * 0.5) * M_PI / 180.0 - M_PI_2;

  hour_points[1].x = cx + cos(hour_angle) * hour_len;
  hour_points[1].y = cy + sin(hour_angle) * hour_len;

  min_points[1].x = cx + cos(min_angle) * min_len;
  min_points[1].y = cy + sin(min_angle) * min_len;

  lv_obj_invalidate(hour_line);
  lv_obj_invalidate(min_line);
}
