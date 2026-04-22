/**
 * @file      event.ino
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2023  Shenzhen Xinyuan Electronic Technology Co., Ltd
 * @date      2023-04-30
 *
 */
#include <LilyGoLib.h>
#include <LV_Helper.h>
#include <WiFi.h>
#include <WiFiManager.h>

#include "timerEvent.h"
#include "watchFace.h"
#include "deviceEvent.h"
#include "dataCollection.h"
#include "accellData.h"
#include "settings.h"
#include "locationData.h"
#include "weatherData.h"
#include "syncNTP.h"
#include "alertData.h"
#include "powerData.h"
#include "ble.h"
#include "config.h"

RTC_DATA_ATTR lv_timer_t watchfaceTimer;
RTC_DATA_ATTR esp_timer_handle_t brightnessTimer;

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

  // Clear all interrupt status
  instance.pmu.clearIrqStatus();
  // Enable the required interrupt function
  instance.pmu.enableIRQ(
    XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |     // BATTERY
    XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |   // VBUS
    XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |      // POWER KEY
    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ  // CHARGE
  );
  Serial.println("setup Interrupts initialized");

  // Accelerator
  // Default 4G ,200HZ
  instance.sensor.configAccelerometer();
  instance.sensor.enableAccelerometer();
  instance.sensor.enablePedometer();
  Serial.println("setup Accelerometer initialized");

  //Enable or Disable PMU Feature
  instance.pmu.enableBattDetection();
  // instance.disableBattDetection();
  instance.pmu.enableVbusVoltageMeasure();
  // instance.disableVbusVoltageMeasure();
  instance.pmu.enableBattVoltageMeasure();
  // instance.disableBattVoltageMeasure();
  instance.pmu.enableSystemVoltageMeasure();
  // instance.disableSystemVoltageMeasure();
  Serial.println("setup PMU feature enabled");

  // Register power event
  // instance.onEvent(device_event_cb, POWER_EVENT, NULL);
  // Register sensor event
  instance.onEvent(device_event_cb);
  Serial.println("setup Power events registered");

  // Set brightness to MAX
  // T-LoRa-Pager brightness level is 0 ~ 16
  // T-Watch-S3 , T-Watch-S3-Plus , T-Watch-Ultra brightness level is 0 ~ 255
  instance.setBrightness(DEVICE_MAX_BRIGHTNESS_LEVEL);
  Serial.println("setup Brightness set to Max");

  // setup NTP time sync
  Serial.println("setup NTP sync setup");
  setupNTPSync();

  // initial operation:
  // get the data initially
  Serial.println("setup Collect data");
  collectData();

  // GUI state
  guiState = WATCHFACE_STATE;
  // draw the watch face initially
  watchFaceSetup();
  drawWatchFace();
/*
  // Problem
  // start timer for watch face
  watchfaceTimer = timerEventWatchface();
*/
  // start timer for minimal brightness
  brightnessTimer = timerEventBrightness();
}

void loop() {
  instance.loop();
  lv_task_handler();
  delay(5);
}
