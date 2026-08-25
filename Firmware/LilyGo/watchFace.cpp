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
#include "timerEvent.h"

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

extern agendaItem allAgendaItems[AGENDA_MAX_NO];
extern int agendaCount;

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
    if ((watchType != QLOCKTWO_WATCH) && (watchType != AGENDA_WATCH))
      drawBattery();

    if (WIFI_CONNECTED) {
      if ((watchType != QLOCKTWO_WATCH) && (watchType != AGENDA_WATCH)) {
        drawWeather();

        drawSolarArc(currentWeather.currentSunrise, currentWeather.currentSunset, currentWeather.currentDT);

        drawUVI(currentWeather.uvi);
      }

      if (newAlertsIndicator == true) {
        drawAlert();
      }
    }

    if ((watchType != QLOCKTWO_WATCH) && (watchType != AGENDA_WATCH))
      drawSteps();
  }

  if (watchType == DIGITAL_WATCH) {
    // digital clock
    drawDate();
    drawTime();
  } else if (watchType == ANALOGUE_WATCH) {
    // analogue clock
    drawAnalogClock();
  } else if (watchType == QLOCKTWO_WATCH) {
    // Qlock 2
    drawQlockTwo();
  } else if (watchType == AGENDA_WATCH) {
    // agenda
    drawAgendaWatchface();
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
  lv_obj_add_style(labelDayWeek, &styleMicro, LV_PART_MAIN);

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

    lv_obj_align(labelDayWeek, LV_ALIGN_TOP_LEFT, 10, 68);
    lv_obj_align(labelDay, LV_ALIGN_TOP_RIGHT, -180, 93);
    lv_obj_align(labelMonth, LV_ALIGN_TOP_LEFT, 65, 95);
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
  lv_obj_align(labelSteps, LV_ALIGN_TOP_LEFT, 60, 192);

  // 3. Der Fortschrittsbalken (Bar)
  lv_obj_t *barSteps = lv_bar_create(screen);

  // Breite anpassen (z.B. 100 Pixel lang, 4 Pixel hoch für einen filigranen Look)
  lv_obj_set_size(barSteps, 80, 4);

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
  lv_obj_add_style(labelLocation, &styleMicro, LV_PART_MAIN);
  // Fester Startpunkt links im unteren Drittel
  lv_obj_align(labelLocation, LV_ALIGN_TOP_LEFT, 10, 145);
  lv_obj_set_style_text_color(labelLocation, GetTheme(THEME_LOCATION_DATA), 0);  // Einheitliche Farbe
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
    lv_obj_align_to(imgWeather, labelLocation, LV_ALIGN_OUT_RIGHT_MID, -13, 0);

    // 3. TEMPERATURE
    lv_obj_t *labelTemperature = lv_label_create(screen);
    lv_obj_add_style(labelTemperature, &styleMicro, LV_PART_MAIN);  // Medium statt Large für eine harmonische Zeile
    lv_obj_set_style_text_color(labelTemperature, weatherColor, 0);

    char buf[6];  // Puffer leicht vergrößert für Sicherheit
    snprintf(buf, sizeof(buf), "%d", currentWeather.temperature);
    lv_label_set_text(labelTemperature, buf);

    // DYNAMISCH: Die Temperatur folgt direkt rechts neben dem Icon
    lv_obj_align_to(labelTemperature, imgWeather, LV_ALIGN_OUT_RIGHT_MID, -13, 0);

    // 4. UNIT (°C / °F)
    lv_obj_t *labelUnit = lv_label_create(screen);
    lv_obj_add_style(labelUnit, &styleMicro, LV_PART_MAIN);
    lv_obj_set_style_text_color(labelUnit, weatherColor, 0);

    if (currentWeather.isMetric)
      lv_label_set_text(labelUnit, "°C");
    else
      lv_label_set_text(labelUnit, "°F");

    // DYNAMISCH: Die Einheit klebt direkt oben rechts neben der Temperatur-Zahl
    lv_obj_align_to(labelUnit, labelTemperature, LV_ALIGN_OUT_RIGHT_TOP, 0, -3);
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

  lv_obj_t *day_label_title = lv_label_create(solar_cont);
  lv_obj_set_style_text_font(day_label_title, &lv_font_montserrat_14, LV_PART_MAIN);
  lv_obj_set_style_text_color(day_label_title, GetTheme(THEME_DATE_DATA), LV_PART_MAIN);
  if (is_night)
    lv_label_set_text(day_label_title, "Night");
  else
    lv_label_set_text(day_label_title, "Day");
  lv_obj_align(day_label_title, LV_ALIGN_CENTER, 0, 25);
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
  lv_color_t main_white = color_text;                              // Knackiges Weiß für Ziffern & Hauptstriche
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

  // Rahmendicke & Farbe (nutzt accent_color oder z. B. lv_color_white())
  lv_obj_set_style_border_width(date_label, 1, 0);
  lv_obj_set_style_border_color(date_label, accent_color, 0);

  // Abgerundete Ecken für das Datumsfenster (z. B. 3px)
  lv_obj_set_style_radius(date_label, 3, 0);

  // Innenabstände (Padding): Macht das Datumsfenster gleichmäßig breit
  lv_obj_set_style_pad_left(date_label, 3, 0);
  lv_obj_set_style_pad_right(date_label, 3, 0);
  lv_obj_set_style_pad_top(date_label, 1, 0);
  lv_obj_set_style_pad_bottom(date_label, 1, 0);

  lv_obj_set_style_min_width(date_label, 22, 0);
  lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, 0);

  // Da die "3" bei -12px vom rechten Rand sitzt, platzieren wir das Datum
  // einfach bei -26px. So steht es wunderschön links daneben!
  lv_obj_align(date_label, LV_ALIGN_RIGHT_MID, -25, 0);

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
  int hours = 0;
  int minutes = 0;
  int day = 0;

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

void drawUVI(double uvi_val) {
  lv_obj_t *uvi_control;
  lv_obj_t *uvi_arc;
  lv_obj_t *uvi_label_value;
  lv_obj_t *uvi_label_title;

  uvi_control = lv_obj_create(screen);
  lv_obj_set_size(uvi_control, 60, 60);
  lv_obj_set_style_bg_opa(uvi_control, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(uvi_control, 0, 0);
  lv_obj_set_style_pad_all(uvi_control, 0, 0);

  lv_obj_align(uvi_control, LV_ALIGN_BOTTOM_RIGHT, -10, -3);

  // 1. Der Haupt-Bogen (Steuert nur die Position des Knobs)
  uvi_arc = lv_arc_create(uvi_control);
  lv_obj_set_size(uvi_arc, 55, 55);  // Leicht angepasst für 5px Stärke
  lv_obj_center(uvi_arc);
  lv_obj_remove_flag(uvi_arc, LV_OBJ_FLAG_CLICKABLE);

  // WICHTIG: Sowohl Hintergrund als auch den aktiven Balken (Indicator) unsichtbar machen!
  lv_obj_set_style_arc_opa(uvi_arc, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(uvi_arc, LV_OPA_TRANSP, LV_PART_INDICATOR);

  // Den KNOB (Zeigerpunkt) aktivieren und stylen
  lv_obj_set_style_bg_opa(uvi_arc, LV_OPA_COVER, LV_PART_KNOB);
  if (uvi_val > 0)
    lv_obj_set_style_bg_color(uvi_arc, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_KNOB);
  else
    lv_obj_set_style_bg_color(uvi_arc, lv_palette_main(LV_PALETTE_GREY), LV_PART_KNOB);
  // Den Punkt perfekt auf die 5px Stärke ausrichten (kein zusätzliches Padding)
  lv_obj_set_style_pad_all(uvi_arc, -2, LV_PART_KNOB);

  lv_arc_set_bg_angles(uvi_arc, 0, 260);
  lv_arc_set_rotation(uvi_arc, 140);
  lv_arc_set_range(uvi_arc, 0, 110);

  // 2. Die 5 Hintergrund-Segmente mit helleren Farben und 5px Stärke
  lv_color_t color_low = lv_palette_main(LV_PALETTE_GREEN);    // Helles Grün
  lv_color_t color_mod = lv_palette_main(LV_PALETTE_YELLOW);   // Helles Gelb
  lv_color_t color_high = lv_palette_main(LV_PALETTE_ORANGE);  // Helles Orange
  lv_color_t color_very = lv_palette_main(LV_PALETTE_RED);     // Helles Rot
  lv_color_t color_ext = lv_palette_main(LV_PALETTE_PURPLE);   // Helles Violett

  for (int i = 0; i < 5; i++) {
    lv_obj_t *background_arc = lv_arc_create(uvi_control);
    lv_obj_set_size(background_arc, 55, 55);
    lv_obj_center(background_arc);
    lv_obj_remove_flag(background_arc, LV_OBJ_FLAG_CLICKABLE);

    lv_obj_set_style_arc_opa(background_arc, LV_OPA_TRANSP, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(background_arc, LV_OPA_TRANSP, LV_PART_KNOB);

    lv_arc_set_bg_angles(background_arc, 0, 52);
    lv_arc_set_rotation(background_arc, 140 + (i * 52));

    // Stärke jetzt auf 5 Pixel erhöht
    lv_obj_set_style_arc_width(background_arc, 5, LV_PART_MAIN);
    // Deckkraft auf 70% hochgesetzt, damit die Farben deutlich kräftiger und heller leuchten
    lv_obj_set_style_arc_opa(background_arc, LV_OPA_70, LV_PART_MAIN);

    if (i == 0) lv_obj_set_style_arc_color(background_arc, color_low, LV_PART_MAIN);
    if (i == 1) lv_obj_set_style_arc_color(background_arc, color_mod, LV_PART_MAIN);
    if (i == 2) lv_obj_set_style_arc_color(background_arc, color_high, LV_PART_MAIN);
    if (i == 3) lv_obj_set_style_arc_color(background_arc, color_very, LV_PART_MAIN);
    if (i == 4) lv_obj_set_style_arc_color(background_arc, color_ext, LV_PART_MAIN);
  }

  // Den Hauptbogen mit dem weißen Zeigerpunkt nach ganz vorne holen
  lv_obj_move_foreground(uvi_arc);

  // 3. Labels (Positionierung bleibt optimiert)
  uvi_label_value = lv_label_create(uvi_control);
  lv_obj_set_style_text_font(uvi_label_value, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(uvi_label_value, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);
  lv_obj_align(uvi_label_value, LV_ALIGN_CENTER, 0, -3);

  uvi_label_title = lv_label_create(uvi_control);
  lv_obj_set_style_text_font(uvi_label_title, &lv_font_montserrat_10, LV_PART_MAIN);
  lv_obj_set_style_text_color(uvi_label_title, GetTheme(THEME_WEATHER_DATA), LV_PART_MAIN);
  lv_label_set_text(uvi_label_title, "UV");
  lv_obj_align(uvi_label_title, LV_ALIGN_CENTER, 0, 15);

  // -----------------------

  int arc_value = (int)(uvi_val * 10.0f);
  if (arc_value > 110) arc_value = 110;
  if (arc_value < 0) arc_value = 0;

  lv_arc_set_value(uvi_arc, arc_value);

  char buf[16];
  lv_snprintf(buf, sizeof(buf), "%.1f", uvi_val);
  lv_label_set_text(uvi_label_value, buf);
}

// Hilfsfunktion: Setze Wortbereich auf aktiv
void set_range(bool mask[10][11], int start_idx, int end_idx) {
  for (int i = start_idx; i <= end_idx; i++) {
    mask[i / 11][i % 11] = true;
  }
}

lv_obj_t *matrix_labels[10][11];
lv_obj_t *corner_dots[4];

// Matrix der Buchstaben
const char *grid = "ITLISASAMPM"
                   "ACQUARTERDC"
                   "TWENTYFIVEX"
                   "HALFSTENFTO"
                   "PASTERUNINE"
                   "ONESIXTHREE"
                   "FOURFIVETWO"
                   "EIGHTELEVEN"
                   "SEVENTWELVE"
                   "TENSEOCLOCK";

void drawQlockTwo() {
  // --------------------
  // get time
  struct tm timeinfo;
  int hour;
  int minute;
  if (!getLocalTime(&timeinfo)) {
    return;
  }
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  hour = timeinfo.tm_hour;
  minute = timeinfo.tm_min;

  // ----------------------------
  // create watch
  lv_obj_t *cont = lv_obj_create(screen);
  lv_obj_set_size(cont, 170, 170);
  lv_obj_align(cont, LV_ALIGN_TOP_LEFT, 35, 35);  // Abstand 10 von links und oben
  lv_obj_set_style_pad_all(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_bg_color(cont, color_bg, 0);
  lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_OFF);

  // Grid-Layout erstellen
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);

  for (int i = 0; i < 110; i++) {
    matrix_labels[i / 11][i % 11] = lv_label_create(cont);
    lv_label_set_text_fmt(matrix_labels[i / 11][i % 11], "%c", grid[i]);
    lv_obj_set_style_text_font(matrix_labels[i / 11][i % 11], &lv_font_unscii_8, 0);
  }

  // Corner Dots (Minimalistisch in den Ecken des 120x120 Bereichs)
  // Erstelle die Dots direkt auf dem Screen, NICHT im cont
  for (int i = 0; i < 4; i++) {
    corner_dots[i] = lv_obj_create(screen);  // statt cont
    lv_obj_set_size(corner_dots[i], 6, 6);
    lv_obj_set_style_radius(corner_dots[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(corner_dots[i], 0, 0);  // Wichtig, damit sie keine Ränder haben
  }
  // Jetzt funktionieren die Align-Befehle, da sie sich auf den Screen beziehen
  int x = 25;
  lv_obj_align(corner_dots[0], LV_ALIGN_TOP_LEFT, x, x);
  lv_obj_align(corner_dots[1], LV_ALIGN_TOP_RIGHT, -x, x);
  lv_obj_align(corner_dots[2], LV_ALIGN_BOTTOM_LEFT, x, -x);
  lv_obj_align(corner_dots[3], LV_ALIGN_BOTTOM_RIGHT, -x, -x);

  // -----------------------
  // update time on watch
  bool mask[10][11] = { false };

  // 1. "IT IS" immer aktiv
  set_range(mask, 0, 1);
  set_range(mask, 3, 4);

  // 2. Minuten-Logik
  int m = (minute / 5) * 5;

  if (m == 5) set_range(mask, 28, 31);   // FIVE
  if (m == 10) set_range(mask, 38, 39);  // TEN
  if (m == 15) set_range(mask, 13, 19);  // QUARTER
  if (m == 20) set_range(mask, 22, 27);  // TWENTY
  if (m == 25) {
    set_range(mask, 22, 27);
    set_range(mask, 28, 31);
  }                                      // TWENTY FIVE
  if (m == 30) set_range(mask, 33, 36);  // HALF
  if (m == 35) {
    set_range(mask, 22, 27);
    set_range(mask, 28, 31);
  }                                          // TWENTY FIVE (vor der Stunde)
  if (m == 40) { set_range(mask, 22, 27); }  // TWENTY (vor der Stunde)
  if (m == 45) { set_range(mask, 13, 19); }  // QUARTER (vor der Stunde)
  if (m == 50) { set_range(mask, 38, 41); }  // TEN (vor der Stunde)
  if (m == 55) { set_range(mask, 28, 31); }  // FIVE (vor der Stunde)

  if (m > 0 && m < 35) set_range(mask, 44, 47);  // PAST
  if (m > 30) set_range(mask, 42, 43);           // TO

  // 3. Stunden-Logik
  // Korrektur: m >= 30, damit bei 12:30 bereits "HALF PAST" und "ONE" (bzw. die nächste Stunde) greift
  int h = (m >= 30) ? (hour + 1) : hour;
  if (h >= 12) h -= 12;  // 13:00 wird zu 1:00, 24:00 zu 0:00
  if (h == 0) h = 12;    // 0:00 oder 12:00 auf 12 setzen

  /* const char* grid="ITLISASAMPM" //  0-10
                      "ACQUARTERDC" // 11-21
                      "TWENTYFIVEX" // 22-32
                      "HALFSTENFTO" // 33-43
                      "PASTERUNINE" // 44-54
                      "ONESIXTHREE" // 55-65
                      "FOURFIVETWO" // 66-76
                      "EIGHTELEVEN" // 77-87
                      "SEVENTWELVE" // 88-98
                      "TENSEOCLOCK";// 99-109 */
  Serial.printf("Stunde: %d, Minute: %d\n", h, m);

  switch (h) {
    case 1: set_range(mask, 55, 57); break;    // ONE
    case 2: set_range(mask, 74, 76); break;    // TWO
    case 3: set_range(mask, 61, 65); break;    // THREE
    case 4: set_range(mask, 66, 69); break;    // FOUR
    case 5: set_range(mask, 70, 73); break;    // FIVE
    case 6: set_range(mask, 58, 60); break;    // SIX
    case 7: set_range(mask, 88, 92); break;    // SEVEN
    case 8: set_range(mask, 77, 81); break;    // EIGHT
    case 9: set_range(mask, 51, 54); break;    // NINE
    case 10: set_range(mask, 99, 101); break;  // TEN
    case 11: set_range(mask, 82, 87); break;   // ELEVEN
    case 12: set_range(mask, 93, 98); break;   // TWELVE
  }

  if (m == 0) set_range(mask, 104, 109);  // OCLOCK

  // UI Update
  for (int r = 0; r < 10; r++) {
    for (int c = 0; c < 11; c++) {
      lv_obj_set_style_text_color(matrix_labels[r][c],
                                  mask[r][c] ? GetTheme(THEME_TIME_DATA) : lv_palette_darken(LV_PALETTE_GREY, 4), 0);
    }
  }

  // Dots
  for (int i = 0; i < 4; i++) {
    lv_obj_set_style_bg_color(corner_dots[i],
                              (i < (minute % 5)) ? GetTheme(THEME_TIME_DATA) : lv_palette_darken(LV_PALETTE_GREY, 4), 0);
  }
}

void drawAgendaHeader(lv_obj_t *parent) {
  lv_obj_t *header = lv_obj_create(parent);
  lv_obj_set_size(header, 240, 40);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);

  // --- Scrollen und Scrollbalken deaktivieren ---
  lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(header, LV_SCROLLBAR_MODE_OFF);

  lv_obj_set_style_bg_opa(header, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(header, 0, 0);
  lv_obj_set_style_pad_left(header, 10, 0);
  lv_obj_set_style_pad_right(header, 10, 0);
  lv_obj_set_style_pad_top(header, 5, 0);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // Uhrzeit links
    lv_obj_t *lbl_time = lv_label_create(header);
    char time_buf[6];
    snprintf(time_buf, sizeof(time_buf), "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    lv_label_set_text(lbl_time, time_buf);
    lv_obj_set_style_text_color(lbl_time, GetTheme(THEME_TIME_DATA), 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_time, LV_ALIGN_LEFT_MID, 5, 5);

    // Datum rechts
    const char *month_names[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
    const char *wday_names[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };

    lv_obj_t *lbl_date = lv_label_create(header);
    char date_buf[20];
    snprintf(date_buf, sizeof(date_buf), "%s, %d %s",
             wday_names[timeinfo.tm_wday],
             timeinfo.tm_mday,
             month_names[timeinfo.tm_mon]);

    lv_label_set_text(lbl_date, date_buf);
    lv_obj_set_style_text_color(lbl_date, GetTheme(THEME_DATE_DATA), 0);
    lv_obj_set_style_text_font(lbl_date, &lv_font_montserrat_20, 0);
    lv_obj_align(lbl_date, LV_ALIGN_RIGHT_MID, -5, 5);
  }
}

#define COLOR_CARD_BG lv_palette_darken(LV_PALETTE_GREY, 4)  // Sehr dunkles Hintergrund-Grau
#define COLOR_TIME lv_palette_darken(LV_PALETTE_GREY, 2)     // Sehr dunkles Hintergrund-Grau
#define COLOR_SUB lv_palette_darken(LV_PALETTE_GREY, 1)      // Sehr dunkles Hintergrund-Grau

#define COLOR_ACTIVE_CARD_BG lv_palette_lighten(LV_PALETTE_GREY, 1)  // Gut lesbares, helles Grau für Dezentes
#define COLOR_ACTIVE_TIME lv_palette_lighten(LV_PALETTE_GREY, 2)     // Gut lesbares, helles Grau für Dezentes
#define COLOR_ACTIVE_SUB lv_palette_lighten(LV_PALETTE_GREY, 4)      // Gut lesbares, helles Grau für Dezentes

void drawAgendaWatchface(void) {
  drawAgendaHeader(screen);

  lv_obj_t *agenda_container = lv_obj_create(screen);
  lv_obj_set_size(agenda_container, 240, 202);
  lv_obj_align(agenda_container, LV_ALIGN_BOTTOM_MID, 0, 0);

  lv_obj_set_flex_flow(agenda_container, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(agenda_container, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  lv_obj_set_style_pad_all(agenda_container, 4, 0);
  lv_obj_set_style_pad_row(agenda_container, 6, 0);
  lv_obj_set_style_bg_opa(agenda_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(agenda_container, 0, 0);

  lv_obj_set_scroll_dir(agenda_container, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(agenda_container, LV_SCROLLBAR_MODE_AUTO);

  registerAgendaEvents(agenda_container);

  // Offset in Sekunden und Millisekunden ermitteln
  int32_t tz_offset_sec = (int32_t)currentWeather.offset;
  int64_t tz_offset_ms = (int64_t)tz_offset_sec * 1000LL;

  time_t now_utc_sec;
  time(&now_utc_sec);

  // Systemzeit in UTC ms
  uint64_t now_ms = (uint64_t)now_utc_sec * 1000ULL;

  int visible_count = 0;

  for (int i = 0; i < AGENDA_MAX_NO; i++) {
    // 1. Ungültige/Leere Eintrags-Prüfung
    if (allAgendaItems[i].startTime == 0 || allAgendaItems[i].endTime == 0) {
      continue;
    }

    // Falls allAgendaItems[] bereits als UTC ms gespeichert ist:
    uint64_t start_utc = allAgendaItems[i].startTime;
    uint64_t end_utc = allAgendaItems[i].endTime;

    // Falls time(&now_utc_sec) bereits Lokalzeit liefert:
    uint64_t now_ms_local = (uint64_t)now_utc_sec * 1000ULL;
    uint64_t now_ms_utc = now_ms_local - tz_offset_ms;  // 2 Stunden abziehen für UTC-Vergleich

    // Jetzt stimmt der Vergleich mit den UTC-Terminen wieder:
    if (now_ms_utc > end_utc) {
      continue;  // Blendet abgelaufene Termine korrekt aus
    }

    visible_count++;
    bool is_current = (now_ms_utc >= start_utc && now_ms_utc <= end_utc);

    // --- TERMIN-KARTE ---
    lv_obj_t *card = lv_obj_create(agenda_container);
    lv_obj_set_width(card, 212);
    lv_obj_set_height(card, LV_SIZE_CONTENT);
    lv_obj_set_style_pad_all(card, 6, 0);
    lv_obj_set_style_pad_row(card, 3, 0);  // Abstand zwischen Zeit, Titel, Bar
    lv_obj_set_style_radius(card, 8, 0);

    // Flex-Layout auf der Karte verhindert Überlappungen
    lv_obj_set_flex_flow(card, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(card, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);

    if (is_current) {
      lv_obj_set_style_bg_color(card, COLOR_ACTIVE_CARD_BG, 0);
      lv_obj_set_style_bg_opa(card, LV_OPA_30, 0);
      lv_obj_set_style_border_color(card, COLOR_ACTIVE_CARD_BG, 0);
      lv_obj_set_style_border_width(card, 2, 0);
    } else {
      lv_obj_set_style_bg_color(card, COLOR_CARD_BG, 0);
      lv_obj_set_style_bg_opa(card, LV_OPA_10, 0);
      lv_obj_set_style_border_color(card, COLOR_CARD_BG, 0);
      lv_obj_set_style_border_width(card, 1, 0);
    }

    // --- 1. Zeit-Label (Umrechnung in lokale Zeit für die Anzeige) ---
    // not for whole day events
    uint64_t total_duration = end_utc - start_utc;
    // 1. Prüfen, ob der UTC-Zeitstempel exakt auf Mitternacht (00:00) fällt
    time_t start_utc_sec = (time_t)(start_utc / 1000ULL);
    struct tm *tm_start_raw = gmtime(&start_utc_sec);
    bool is_midnight_start = (tm_start_raw && tm_start_raw->tm_hour == 0 && tm_start_raw->tm_min == 0);
    // Ganztägig = Startet um 00:00 UTC und dauert mindestens 24 Stunden (86.400.000 ms)
    bool is_all_day = is_midnight_start && (total_duration >= 86400000ULL);

    time_t start_local_sec = (time_t)(start_utc / 1000ULL);
    time_t end_local_sec = (time_t)(end_utc / 1000ULL);
    if (!is_all_day) {
      start_local_sec = start_local_sec + tz_offset_sec;
      end_local_sec = end_local_sec + tz_offset_sec;
    }

    struct tm *tm_start = gmtime(&start_local_sec);
    char start_str[6];
    if (tm_start) {
      strftime(start_str, sizeof(start_str), "%H:%M", tm_start);
    } else {
      snprintf(start_str, sizeof(start_str), "--:--");
    }

    struct tm *tm_end = gmtime(&end_local_sec);
    char end_str[6];
    if (tm_end) {
      strftime(end_str, sizeof(end_str), "%H:%M", tm_end);
    } else {
      snprintf(end_str, sizeof(end_str), "--:--");
    }

    char time_buf[32];
    snprintf(time_buf, sizeof(time_buf), "%s %s - %s", is_current ? "• LIVE" : "•", start_str, end_str);

    lv_obj_t *lbl_time = lv_label_create(card);
    lv_label_set_text(lbl_time, time_buf);
    lv_obj_set_style_text_color(lbl_time, is_current ? COLOR_ACTIVE_TIME : COLOR_TIME, 0);
    lv_obj_set_style_text_font(lbl_time, &lv_font_montserrat_12, 0);

    // --- 2. Titel-Label ---
    lv_obj_t *lbl_title = lv_label_create(card);
    lv_label_set_text(lbl_title, allAgendaItems[i].subject);
    lv_label_set_long_mode(lbl_title, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lbl_title, 196);
    lv_obj_set_style_text_color(lbl_title, is_current ? COLOR_ACTIVE_SUB : COLOR_SUB, 0);
    lv_obj_set_style_text_font(lbl_title, &lv_font_montserrat_12, 0);

    // --- 3. Fortschrittsbalken für aktive Termine ---
    if (is_current) {
      uint64_t total_duration = end_utc - start_utc;
      int progress = 0;

      const uint64_t ONE_DAY_MS = 86400000ULL;

      if (total_duration > ONE_DAY_MS) {
        // Mehrtägiger Termin: Fortschritt basierend auf dem heutigen Tag berechnen
        time_t now_sec = (time_t)(now_ms_utc / 1000ULL) + tz_offset_sec;
        struct tm *tm_now = gmtime(&now_sec);

        uint64_t day_start_ms = now_ms_utc - ((tm_now->tm_hour * 3600 + tm_now->tm_min * 60 + tm_now->tm_sec) * 1000ULL);
        uint64_t day_elapsed = (now_ms_utc > day_start_ms) ? (now_ms_utc - day_start_ms) : 0;

        progress = (int)((day_elapsed * 100ULL) / ONE_DAY_MS);
      } else if (total_duration > 0) {
        // Normaler Tages-Termin: Regulärer Fortschritt
        uint64_t elapsed = now_ms_utc - start_utc;
        progress = (int)((elapsed * 100ULL) / total_duration);
      }

      // Begrenzung & Mindestbreite (mind. 5% sichtbare Füllung bei aktiven Terminen)
      if (progress < 5) progress = 5;
      if (progress > 100) progress = 100;

      lv_obj_t *bar = lv_bar_create(card);
      lv_obj_set_size(bar, 196, 4);  // Reduziert von 210 auf 196
      lv_bar_set_value(bar, progress, LV_ANIM_OFF);
      lv_obj_set_style_bg_color(bar, lv_palette_darken(LV_PALETTE_GREY, 2), LV_PART_MAIN);
      lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    }
  }

  // Leer-Zustand
  if (visible_count == 0) {
    lv_obj_t *empty_card = lv_obj_create(agenda_container);
    lv_obj_set_width(empty_card, 228);
    lv_obj_set_height(empty_card, 70);
    lv_obj_set_style_bg_color(empty_card, COLOR_CARD_BG, 0);
    lv_obj_set_style_bg_opa(empty_card, LV_OPA_10, 0);
    lv_obj_set_style_border_width(empty_card, 1, 0);
    lv_obj_set_style_border_color(empty_card, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_radius(empty_card, 8, 0);

    lv_obj_t *lbl_empty = lv_label_create(empty_card);
    lv_label_set_text(lbl_empty, "No remaining agenda items.");
    lv_obj_set_style_text_align(lbl_empty, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_color(lbl_empty, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_set_style_text_font(lbl_empty, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl_empty);
  }
}

// sub CB: Default
static void eventGestureAgendaCB(lv_event_t *e) {
  // 1. Den Event-Code abrufen
  lv_event_code_t code = lv_event_get_code(e);

  // 2. Den Code prüfen
  if ((code == LV_EVENT_SCROLL_END) || (code == LV_EVENT_GESTURE) || (code == LV_EVENT_SCROLL)) {

    const char *name = lv_event_code_get_name(code);
    if (name != NULL) {
      Serial.print("Event code / name: ");
      Serial.print(code, DEC);
      Serial.print(" / ");
      Serial.println(name);
    } else {
      Serial.print("Event code: ");
      Serial.println(code, DEC);
    }

    startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);
  }
}

static void registerAgendaEvents(lv_obj_t *cont) {
  lv_obj_add_event_cb(cont, eventGestureAgendaCB, LV_EVENT_SCROLL_END, NULL);
  lv_obj_add_event_cb(cont, eventGestureAgendaCB, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(cont, eventGestureAgendaCB, LV_EVENT_SCROLL, NULL);
}
