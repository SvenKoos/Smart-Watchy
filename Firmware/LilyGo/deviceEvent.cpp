#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "dataCollection.h"
#include "alertData.h"
#include "accellData.h"
#include "deviceEvent.h"
#include "config.h"
#include "watchFace.h"
#include "timerEvent.h"
#include "menuHandler.h"
#include "settings.h"

extern uint32_t stepCounter;
extern alertData currentAlerts;
extern singleAlert allAlerts[ALERT_MAX_NO];
extern int guiState;
extern accellData currentAccelleration;
extern bool newAlertsIndicator;

extern lv_color_t color_bg;
extern lv_color_t color_text;

int alertIndex = -1;

lv_style_t styleAlerts;

lv_obj_t* labelTime;
lv_obj_t* labelApp;
lv_obj_t* labelTitleBody;
lv_obj_t *dots_container = NULL; // Speichert den Container für die Punkte

uint32_t last_gesture_time = 0;

LV_FONT_DECLARE(emoji);
static lv_font_t watchface_font;

void setupDeviceEvent() {
  instance.onEvent(device_event_cb);

  // Wir kopieren die Struktur der eingebauten Montserrat-Schriftart in unser neues Objekt
  watchface_font = lv_font_montserrat_18;
  // Da 'watchface_font' im RAM liegt, dürfen wir hier jetzt den Fallback setzen!
  watchface_font.fallback = &emoji;
  Serial.println("Custom Watchface-Font mit Emoji-Fallback bereit!");
}

static void device_event_cb(DeviceEvent_t event, void* params, void* user_data) {
  if (event == POWER_EVENT) {
    switch (instance.getPMUEventType(params)) {
      case PMU_EVENT_BATTERY_LOW_TEMP:
        // Serial.println("Battery temperature is low");
        break;
      case PMU_EVENT_BATTERY_HIGH_TEMP:
        // Serial.println("Battery temperature is very high");
        break;
      case PMU_EVENT_CHARGE_LOW_TEMP:
        // Serial.println("Charger temperature is low");
        break;
      case PMU_EVENT_CHARGE_HIGH_TEMP:
        // Serial.println("Charger temperature is high");
        break;
      case PMU_EVENT_LOW_VOLTAGE_LEVEL1:
        // Serial.println("Low battery low voltage warning level 1");
        break;
      case PMU_EVENT_LOW_VOLTAGE_LEVEL2:
        // Serial.println("Low battery low voltage warning level 2");
        break;
      case PMU_EVENT_KEY_CLICKED:
        Serial.println("Power button is clicked");

        // set brightness
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        handle_button_emergency_reset();

        if (guiState == DARK_STATE) {
          guiState = WATCHFACE_STATE;
          currentAccelleration.isMoved = true;
          drawWatchFace();
        } else if (guiState == WATCHFACE_STATE) {
          // GUI state
          guiState = MENU_STATE;

          // call the menu
          menuHandler();
        }

        break;
      case PMU_EVENT_KEY_LONG_PRESSED:
        // Serial.println("Power button is long-pressed");
        break;
      case PMU_EVENT_BATTERY_REMOVE:
        // Serial.println("Battery is removed");
        break;
      case PMU_EVENT_BATTERY_INSERT:
        // Serial.println("Battery is inserted");
        break;
      case PMU_EVENT_USBC_REMOVE:
        // Serial.println("Power adapter removed");

        // draw the watchface screen
        guiState = WATCHFACE_STATE;
        currentAccelleration.isMoved = true;
        drawWatchFace();

        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        break;
      case PMU_EVENT_USBC_INSERT:
        // Serial.println("Power adapter plugged in");

        // draw the watchface screen
        guiState = WATCHFACE_STATE;
        currentAccelleration.isMoved = true;
        drawWatchFace();

        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        break;
      case PMU_EVENT_BATTERY_OVER_VOLTAGE:
        // Serial.println("Battery over-voltage protection warning");
        break;
      case PMU_EVENT_CHARGE_TIMEOUT:
        // Serial.println("Battery charging timeout");
        break;
      case PMU_EVENT_CHARGE_STARTED:
        // Serial.println("Battery charging starts");
        break;
      case PMU_EVENT_CHARGE_FINISH:
        // Serial.println("Battery charging finish");
        break;
      case PMU_EVENT_BAT_FET_OVER_CURRENT:
        // Serial.println("Battery FET over-current detected");
        break;
      default:
        break;
    }
  } else if (event == SENSOR_EVENT) {
    switch (instance.getSensorEventType(params)) {
      case SENSOR_STEPS_UPDATED:
        stepCounter = instance.sensor.getPedometerCounter();
        Serial.printf("Step count interrupt,step Counter:%u\n", stepCounter);
        break;
      case SENSOR_ACTIVITY_DETECTED:
        // Serial.println("Activity event");
        break;
      case SENSOR_TILT_DETECTED:
        // Serial.println("Tilt event");
        break;
      case SENSOR_DOUBLE_TAP_DETECTED:
        Serial.println("DoubleTap event");

        if (guiState == WATCHFACE_STATE) {
          Serial.println("DoubleTap event: watchface state");

          // show the  alerts
          if (currentAlerts.count > 0) {
            // prepare the  screen object
            prepareAlertScreen(currentAlerts.count);

            // GUI state
            guiState = ALERT_STATE;

            // alert
            alertIndex = currentAlerts.count - 1;
            showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);

            newAlertsIndicator = false;
          }
        } else if (guiState == ALERT_STATE) {
          Serial.println("DoubleTap event: alert state");

          guiState = WATCHFACE_STATE;

          // draw the watchface screen
          drawWatchFace();
        } else if (guiState == DARK_STATE) {
          Serial.println("DoubleTap event: dark state");

          guiState = WATCHFACE_STATE;
          currentAccelleration.isMoved = true;
          drawWatchFace();
        }

        // set brightness
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        break;
      case SENSOR_ANY_MOTION_DETECTED:
        // Serial.println("Any motion / no motion event");
        break;
      default:
        break;
    }
  }
}

static void alertEventCB(lv_event_t* e) {
  // set brightness
  startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

  if ((guiState == ALERT_STATE) && (lv_event_get_code(e) == LV_EVENT_GESTURE)) {
    // 500ms Sperrzeit nach der letzten erfolgreichen Geste
    if (lv_tick_elaps(last_gesture_time) < 500) {
      return;
    }

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    switch (dir) {
      case LV_DIR_LEFT:
        last_gesture_time = lv_tick_get();  // Zeitstempel setzen

        if (currentAlerts.count > 0) {
          if (alertIndex < currentAlerts.count - 1) {
            alertIndex++;
            showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
          }
        }
        break;
      case LV_DIR_RIGHT:
        last_gesture_time = lv_tick_get();  // Zeitstempel setzen

        if (currentAlerts.count > 0) {
          if (alertIndex > 0) {
            alertIndex--;
            showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
          }
        }
        break;
      case LV_DIR_TOP:
        break;
      case LV_DIR_BOTTOM:
        break;
    }
  }
}

lv_obj_t* prepareAlertScreen(int count) {
  // Active Screen holen und säubern
  lv_obj_t* alert_scr = lv_screen_active();
  lv_obj_clean(alert_scr);
  lv_obj_set_style_bg_color(alert_scr, color_bg, 0);

  // Vertikales Scrollen aktivieren
  lv_obj_add_flag(alert_scr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(alert_scr, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_add_event_cb(alert_scr, alertEventCB, LV_EVENT_GESTURE, NULL);

  // --- HEADER AREA ---
  static lv_style_t styleHeader;
  lv_style_init(&styleHeader);
  lv_style_set_text_font(&styleHeader, &lv_font_montserrat_18);
  lv_style_set_text_color(&styleHeader, lv_palette_lighten(LV_PALETTE_GREY, 1));

  // App Name
  labelApp = lv_label_create(alert_scr);
  lv_obj_add_style(labelApp, &styleHeader, LV_PART_MAIN);
  lv_obj_set_style_text_color(labelApp, GetTheme(THEME_ALERT_DATA), 0);
  lv_obj_align(labelApp, LV_ALIGN_TOP_LEFT, 12, 10);

  // time
  labelTime = lv_label_create(alert_scr);
  lv_obj_add_style(labelTime, &styleHeader, LV_PART_MAIN);
  lv_obj_set_style_text_align(labelTime, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(labelTime, LV_ALIGN_TOP_RIGHT, -12, 10);

  // --- DYNAMISCHE PUNKT-ZEILE (INDEX ANZEIGE) ---
  dots_container = lv_obj_create(alert_scr);
  lv_obj_set_size(dots_container, lv_pct(100), 12);
  lv_obj_align(dots_container, LV_ALIGN_TOP_MID, 0, 40); // Zwischen Header und Karte
  
  lv_obj_set_style_bg_opa(dots_container, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(dots_container, 0, 0);
  lv_obj_set_style_pad_all(dots_container, 0, 0);
  lv_obj_remove_flag(dots_container, LV_OBJ_FLAG_SCROLLABLE);
  
  // Flex-Layout für automatische horizontale Zentrierung
  lv_obj_set_flex_flow(dots_container, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_flex_main_place(dots_container, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_flex_cross_place(dots_container, LV_FLEX_ALIGN_CENTER, 0);
  lv_obj_set_style_pad_column(dots_container, 6, 0);

  // Punkte im Ausgangszustand (alle dunkelgrau) zeichnen
  for(int i = 0; i < count; i++) {
    lv_obj_t *dot = lv_obj_create(dots_container);
    lv_obj_set_size(dot, 5, 5);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(dot, 0, 0);
    lv_obj_set_style_bg_color(dot, lv_palette_darken(LV_PALETTE_GREY, 4), 0); // Alle dunkelgrau
  }

  // --- CONTENT CONTAINER ("Die Message-Karte") ---
  lv_obj_t *card = lv_obj_create(alert_scr);
  lv_obj_set_size(card, lv_pct(92), LV_SIZE_CONTENT);
  lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 54); // Platz für die Punkte gelassen
  
  lv_obj_set_style_bg_color(card, lv_palette_darken(LV_PALETTE_BLUE_GREY, 4), 0);
  lv_obj_set_style_radius(card, 8, 0);
  lv_obj_set_style_border_width(card, 0, 0);
  lv_obj_set_style_pad_all(card, 10, 0);
  lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE); 

  // --- VERTICAL ACCENT BAR ---
  lv_obj_t *accent_bar = lv_obj_create(card);
  lv_obj_set_size(accent_bar, 3, lv_pct(100));
  lv_obj_align(accent_bar, LV_ALIGN_LEFT_MID, -4, 0);
  lv_obj_set_style_bg_color(accent_bar, GetTheme(THEME_ALERT_DATA), 0);
  lv_obj_set_style_border_width(accent_bar, 0, 0);
  lv_obj_set_style_radius(accent_bar, 2, 0);

  // --- MESSAGE TEXT ---
  labelTitleBody = lv_label_create(card);
  lv_obj_set_style_text_font(labelTitleBody, &watchface_font, LV_PART_MAIN);
  lv_obj_set_style_text_color(labelTitleBody, lv_color_white(), LV_PART_MAIN);
  
  lv_label_set_long_mode(labelTitleBody, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(labelTitleBody, lv_pct(92));
  lv_obj_align(labelTitleBody, LV_ALIGN_LEFT_MID, 8, 0);

  last_gesture_time = 0;

  return alert_scr;
}

void showAlert(singleAlert alert, int index, int count) {
  char text[TITLE_LEN + BODY_LEN] = "";

  lv_label_set_text_fmt(labelTime, "%.5s", alert.timeStamp + 11);

  // 3. App-Name setzen
  lv_label_set_text(labelApp, alert.appName);

  // 4. Inhalt (Titel + Body) zusammenbauen
  strcpy(text, alert.title);
  strcat(text, "\n");
  strcat(text, alert.body);
  lv_label_set_text(labelTitleBody, text);

  // 5. AKTUELLEN PUNKT HELLGRAU INTEGRIEREN
  if (dots_container != NULL) {
    uint32_t child_count = lv_obj_get_child_count(dots_container);
    
    for(uint32_t i = 0; i < child_count; i++) {
      lv_obj_t *dot = lv_obj_get_child(dots_container, i);
      
      if (i == index) {
        lv_obj_set_style_bg_color(dot, lv_palette_lighten(LV_PALETTE_GREY, 2), 0); // Aktueller Index = Hellgrau
      } else {
        lv_obj_set_style_bg_color(dot, lv_palette_darken(LV_PALETTE_GREY, 4), 0);   // Alle anderen = Dunkelgrau
      }
    }
  }
}

static void handle_button_emergency_reset() {
  // 1. Check: Reagiert der Touch auf I2C?
  Wire.beginTransmission(0x38);
  if (Wire.endTransmission() != 0) {
    Serial.println("Touch frozen! Power-cycling DLDO1...");

    // Die korrekten Public-Funktionen für den AXP2101:
    instance.pmu.disableDLDO1();
    delay(100);

    // Spannung sicherheitshalber setzen und wieder einschalten
    instance.pmu.setDLDO1Voltage(3300);
    instance.pmu.enableDLDO1();

    delay(200);  // Warten auf Chip-Boot

    if (instance.begin()) {
      Serial.println("handle_button_emergency_reset Touch recovered!");
    }
  }
}
