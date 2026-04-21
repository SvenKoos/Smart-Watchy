#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>

#include "dataCollection.h"
#include "deviceEvent.h"
#include "alertData.h"
#include "config.h"
#include "watchFace.h"

extern lv_timer_t brightnessTimer;
extern uint32_t stepCounter;
extern alertData currentAlerts;
extern singleAlert allAlerts[ALERT_MAX_NO];
extern int guiState;

RTC_DATA_ATTR int alertIndex = -1;
RTC_DATA_ATTR lv_obj_t* screenAlerts;
RTC_DATA_ATTR lv_style_t styleAlerts;

void device_event_cb(DeviceEvent_t event, void* params, void* user_data) {
  if (event != POWER_EVENT) {
    switch (instance.getPMUEventType(params)) {
      case PMU_EVENT_BATTERY_LOW_TEMP:
        Serial.println("Battery temperature is low");
        break;
      case PMU_EVENT_BATTERY_HIGH_TEMP:
        Serial.println("Battery temperature is very high");
        break;
      case PMU_EVENT_CHARGE_LOW_TEMP:
        Serial.println("Charger temperature is low");
        break;
      case PMU_EVENT_CHARGE_HIGH_TEMP:
        Serial.println("Charger temperature is high");
        break;
      case PMU_EVENT_LOW_VOLTAGE_LEVEL1:
        Serial.println("Low battery low voltage warning level 1");
        break;
      case PMU_EVENT_LOW_VOLTAGE_LEVEL2:
        Serial.println("Low battery low voltage warning level 2");
        break;
      case PMU_EVENT_KEY_CLICKED:
        Serial.println("Power button is clicked");

        // set brightness
        instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
        lv_timer_resume(&brightnessTimer);
        lv_timer_reset(&brightnessTimer);

        // GUI state
        // guiState = MENU_STATE;

        // call the menu

        break;
      case PMU_EVENT_KEY_LONG_PRESSED:
        Serial.println("Power button is long-pressed");
        break;
      case PMU_EVENT_BATTERY_REMOVE:
        Serial.println("Battery is removed");
        break;
      case PMU_EVENT_BATTERY_INSERT:
        Serial.println("Battery is inserted");
        break;
      case PMU_EVENT_USBC_REMOVE:
        Serial.println("Power adapter removed");
        break;
      case PMU_EVENT_USBC_INSERT:
        Serial.println("Power adapter plugged in");
        break;
      case PMU_EVENT_BATTERY_OVER_VOLTAGE:
        Serial.println("Battery over-voltage protection warning");
        break;
      case PMU_EVENT_CHARGE_TIMEOUT:
        Serial.println("Battery charging timeout");
        break;
      case PMU_EVENT_CHARGE_STARTED:
        Serial.println("Battery charging starts");
        break;
      case PMU_EVENT_CHARGE_FINISH:
        Serial.println("Battery charging finish");
        break;
      case PMU_EVENT_BAT_FET_OVER_CURRENT:
        Serial.println("Battery FET over-current detected");
        break;
      default:
        break;
    }
  } else

    if (event == SENSOR_EVENT) {
    switch (instance.getSensorEventType(params)) {
      case SENSOR_STEPS_UPDATED:
        stepCounter = instance.sensor.getPedometerCounter();
        Serial.printf("Step count interrupt,step Counter:%u\n", stepCounter);
        break;
      case SENSOR_ACTIVITY_DETECTED:
        Serial.println("Activity event");
        break;
      case SENSOR_TILT_DETECTED:
        Serial.println("Tilt event");
        break;
      case SENSOR_DOUBLE_TAP_DETECTED:
        Serial.println("DoubleTap event");

        // set brightness
        instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
        lv_timer_resume(&brightnessTimer);
        lv_timer_reset(&brightnessTimer);

        if (guiState == WATCHFACE_STATE) {
          // show the  alerts
          if (currentAlerts.count > 0) {
            // GUI state
            guiState = ALERT_STATE;

            // screen
            // Create the new screen object
            screenAlerts = lv_obj_create(NULL);
            // register gestures
            // https://docs.lvgl.io/master/main-modules/indev/gestures.html
            lv_obj_add_event_cb(screenAlerts, alertEventCB, LV_EVENT_GESTURE, NULL);
            // Load the screen
            lv_scr_load(screenAlerts);

            // style
            lv_style_init(&styleAlerts);
            lv_style_set_text_font(&styleAlerts, &lv_font_montserrat_10);
            lv_style_set_bg_opa(&styleAlerts, LV_OPA_TRANSP);
            lv_style_set_text_color(&styleAlerts, lv_color_black());
            lv_style_set_border_width(&styleAlerts, 0);

            // alert
            alertIndex = 0;
            showAlert(allAlerts[alertIndex], screenAlerts, styleAlerts);
          } else if (guiState == ALERT_STATE) {
            guiState = WATCHFACE_STATE;
            drawWatchFace();
          } else if (guiState == DARK_STATE) {
            guiState = WATCHFACE_STATE;
          }
        }

        break;
      case SENSOR_ANY_MOTION_DETECTED:
        Serial.println("Any motion / no motion event");
        break;
      default:
        break;
    }
  }
}

void alertEventCB(lv_event_t* e) {
  // set brightness
  instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
  lv_timer_resume(&brightnessTimer);
  lv_timer_reset(&brightnessTimer);

  if (guiState == ALERT_STATE) {
    // lv_obj_t * screen = (lv_obj_t*) lv_event_get_current_target(e);

    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
    switch (dir) {
      case LV_DIR_LEFT:
        //
        break;
      case LV_DIR_RIGHT:
        //
        break;
      case LV_DIR_TOP:
        if (currentAlerts.count > 0) {
          if (alertIndex < currentAlerts.count - 1) {
            alertIndex++;
            showAlert(allAlerts[alertIndex], screenAlerts, styleAlerts);
          }
        }
        break;
      case LV_DIR_BOTTOM:
        if (currentAlerts.count > 0) {
          if (alertIndex > 0) {
            alertIndex--;
            showAlert(allAlerts[alertIndex], screenAlerts, styleAlerts);
          }
        }
        break;
    }
  }
}
