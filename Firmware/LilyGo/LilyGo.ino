#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "dataCollection.h"
#include "alertData.h"
#include "deviceEvent.h"
#include "accellData.h"
#include "settings.h"
#include "locationData.h"
#include "weatherData.h"
#include "syncNTP.h"
#include "powerData.h"
#include "ble.h"
#include "config.h"
#include "timerEvent.h"

RTC_DATA_ATTR bool update_gui_request;

RTC_DATA_ATTR uint32_t stepCounter;

RTC_DATA_ATTR locationData currentLocation;

RTC_DATA_ATTR alertData currentAlerts;
RTC_DATA_ATTR singleAlert allAlerts[ALERT_MAX_NO];

RTC_DATA_ATTR accellData currentAccelleration;

RTC_DATA_ATTR bool WIFI_CONFIGURED = false;
RTC_DATA_ATTR bool WIFI_CONNECTED = false;

RTC_DATA_ATTR int guiState = -1;

RTC_DATA_ATTR lilygoSettings settings;

RTC_DATA_ATTR weatherData currentWeather;

RTC_DATA_ATTR powerData currentPower;

RTC_DATA_ATTR bool bleBonded = false;

void setup() {
  Serial.begin(115200);

  instance.begin();
  Serial.println("setup Instance started");

  beginLvglHelper(instance);
  Serial.println("setup LVGL helper started");

  // take settings
  settings = setSetting();
  Serial.println("setup Settings loaded");

  // Accelerator
  setupAccellData();
  Serial.println("setup Accelerometer");

  // Register power event
  // Register sensor event
  setupDeviceEvent();
  Serial.println("setup Power events registered");

  // setup power management
  setupPowerMgt();
  Serial.println("setup power management");

  // Set brightness to MAX
  // T-LoRa-Pager brightness level is 0 ~ 16
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  displayWakup();
  Serial.println("setup Brightness set to Max");

  // initial operation:
  // get the data initially
  Serial.println("setup Collect data");
  collectData();

  // GUI state
  guiState = WATCHFACE_STATE;
  // draw the watch face initially
  watchFaceSetup();
  drawWatchFace();

  // start timer for watch face - Problem
  update_gui_request = false;
  timerEventWatchface();

  // start timer for minimal brightness
  timerEventBrightness(10);
}

void loop() {
  instance.loop();

  if (guiState != DARK_STATE) {
    // lv_task_handler();
    lv_timer_handler();  // Verarbeitet die Timer
  }

  // 2. Prüfen, ob der Hardware-Timer die Flag gesetzt hat
  if (update_gui_request) {
    Serial.println("loop update watchface");

    update_gui_request = false;  // Flag sofort zurücksetzen

    // get the data
    collectData();

    // verify power mgt setup
    verifyPowerMgt();

    if (guiState == WATCHFACE_STATE) {
      // draw watch face
      drawWatchFace();
    }
  }

  delay(10);
}
