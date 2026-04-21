#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include <WiFiManager.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include <Arduino_JSON.h>
#include <Time.h>
#include <TimeLib.h>
#include <esp_sntp.h>

#include "config.h"
#include "dataCollection.h"
#include "accellData.h"

extern accellData currentAccelleration;

accellData getAccellData() {
  Serial.println("getAccellData Start");

  // reset step counter at midnight
  struct tm timeinfo;
  // Get the time C library structure
  instance.rtc.getDateTime(&timeinfo);

  if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0) {
    resetAccellData();
  }

  int16_t oldAccelX = currentAccelleration.xAccell;
  int16_t oldAccelY = currentAccelleration.yAccell;
  int16_t oldAccelZ = currentAccelleration.zAccell;

  instance.sensor.getAccelerometer(currentAccelleration.xAccell, currentAccelleration.yAccell, currentAccelleration.zAccell);
  currentAccelleration.stepCounter = instance.sensor.getPedometerCounter();

  currentAccelleration.isMoved = ((abs(oldAccelX - currentAccelleration.xAccell) > MAX_ACCEL_QUIET) || (abs(oldAccelY - currentAccelleration.yAccell) > MAX_ACCEL_QUIET) || (abs(oldAccelZ - currentAccelleration.zAccell) > MAX_ACCEL_QUIET));

  currentAccelleration.code = CODE_NO_ERROR;

  Serial.print("getAccellData X: ");
  Serial.print(currentAccelleration.xAccell, DEC);
  Serial.print(" Y: ");
  Serial.print(currentAccelleration.yAccell, DEC);
  Serial.print(" Z: ");
  Serial.println(currentAccelleration.zAccell, DEC);

  return currentAccelleration;
}

void resetAccellData() {
  instance.sensor.resetPedometer();
}
