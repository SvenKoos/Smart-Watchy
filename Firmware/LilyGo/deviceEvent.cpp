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

extern uint32_t stepCounter;
extern alertData currentAlerts;
extern singleAlert allAlerts[ALERT_MAX_NO];
extern int guiState;
extern accellData currentAccelleration;

int alertIndex = -1;

lv_style_t styleAlerts;

lv_obj_t* labelTimestamp;
lv_obj_t* labelIndex;
lv_obj_t* labelApp;
lv_obj_t* labelTitleBody;

uint32_t last_gesture_time = 0;

void setupDeviceEvent() {
  instance.onEvent(device_event_cb);
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
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        // draw the watchface screen
        guiState = WATCHFACE_STATE;
        currentAccelleration.isMoved = true;
        drawWatchFace();

        break;
      case PMU_EVENT_USBC_INSERT:
        // Serial.println("Power adapter plugged in");
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        // draw the watchface screen
        guiState = WATCHFACE_STATE;
        currentAccelleration.isMoved = true;
        drawWatchFace();

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

        // set brightness
        displayWakup();
        startBrightnessTimer(BRIGHTNESS_TIMEOUT_DEFAULT);

        if (guiState == WATCHFACE_STATE) {
          Serial.println("DoubleTap event: watchface state");

          // show the  alerts
          if (currentAlerts.count > 0) {
            // prepare the  screen object
            prepareAlertScreen();

            // GUI state
            guiState = ALERT_STATE;

            // alert
            alertIndex = currentAlerts.count - 1;
            showAlert(allAlerts[alertIndex], alertIndex, currentAlerts.count);
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
  displayWakup();
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

lv_obj_t* prepareAlertScreen() {
  // get active screen
  lv_obj_t* alert_scr = lv_screen_active();

  // clean the screen
  lv_obj_clean(alert_scr);

  // 2. Scroll-Verhalten aktivieren
  // Wir erlauben vertikales Scrollen, schalten aber horizontales aus
  lv_obj_add_flag(alert_scr, LV_OBJ_FLAG_SCROLLABLE);

  // Scrollbalken dezent anzeigen (nur während des Scrollens)
  lv_obj_set_scrollbar_mode(alert_scr, LV_SCROLLBAR_MODE_AUTO);

  // register gestures
  // https://docs.lvgl.io/master/main-modules/indev/gestures.html
  lv_obj_add_event_cb(alert_scr, alertEventCB, LV_EVENT_GESTURE, NULL);

  // style
  lv_style_init(&styleAlerts);
  lv_style_set_text_font(&styleAlerts, &lv_font_montserrat_18);
  lv_style_set_border_width(&styleAlerts, 0);

  // timestamp
  labelTimestamp = lv_label_create(alert_scr);
  lv_obj_add_style(labelTimestamp, &styleAlerts, LV_PART_MAIN);
  lv_obj_align(labelTimestamp, LV_ALIGN_TOP_LEFT, 5, 5);

  // Index
  labelIndex = lv_label_create(alert_scr);
  lv_obj_add_style(labelIndex, &styleAlerts, LV_PART_MAIN);
  lv_obj_set_style_text_align(labelIndex, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(labelIndex, LV_ALIGN_TOP_RIGHT, -10, 5);

  // app
  labelApp = lv_label_create(alert_scr);
  lv_obj_add_style(labelApp, &styleAlerts, LV_PART_MAIN);
  lv_obj_align(labelApp, LV_ALIGN_TOP_LEFT, 5, 30);

  // title + body
  labelTitleBody = lv_label_create(alert_scr);
  lv_obj_add_style(labelTitleBody, &styleAlerts, LV_PART_MAIN);
  // WICHTIG: Long Mode auf WRAP setzen, damit der Text in die Breite passt
  // und stattdessen die Höhe des Objekts wächst
  lv_label_set_long_mode(labelTitleBody, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(labelTitleBody, lv_pct(95));  // 90% der Screenbreite nutzen
  lv_obj_align(labelTitleBody, LV_ALIGN_TOP_LEFT, 5, 55);

  last_gesture_time = 0;

  return alert_scr;
}

void showAlert(singleAlert alert, int index, int count) {
  char text[TITLE_LEN + BODY_LEN] = "";

  lv_label_set_text_fmt(labelTimestamp, "%.16s", alert.timeStamp);

  lv_label_set_text_fmt(labelIndex, "%d / %d", index + 1, count);

  lv_label_set_text(labelApp, alert.appName);

  // lv_label_set_text(labelTitle, alert.title);
  strcpy(text, alert.title);
  strcat(text, "\n");
  strcat(text, alert.body);
  lv_label_set_text(labelTitleBody, text);
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
