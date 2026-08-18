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
extern uint16_t stepCounterHistory[1440];

accellData getAccellData() {
  Serial.println("getAccellData Start");

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

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // store current step counter value
    int currentMinuteIndex = 0;
    currentMinuteIndex = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    stepCounterHistory[currentMinuteIndex] = currentAccelleration.stepCounter;
  }

  return currentAccelleration;
}

void resetAccellData() {
  // reset step counter at midnight
  for (int i = 0; i < 1440; i++) {
    stepCounterHistory[i] = 0;
  }

  instance.sensor.resetPedometer();
}

void setupAccellData() {
  // step counter history
  for (int i = 0; i < 1440; i++) {
    stepCounterHistory[i] = 0;
  }

  // Default 4G ,200HZ
  instance.sensor.configAccelerometer();
  instance.sensor.enableAccelerometer();
  instance.sensor.enablePedometer();

  instance.sensor.configInterrupt();

  // NUR die Features aktivieren, die wir wirklich wollen.
  // FEATURE_WAKEUP ist bei LilyGo oft das Synonym für Double Tap.
  // Wir entfernen: ANY_MOTION, NO_MOTION, ACTIVITY und TILT.
  instance.sensor.enableFeature(SensorBMA423::FEATURE_STEP_CNTR | SensorBMA423::FEATURE_WAKEUP, true);

  // INTERRUPTS: Hier entscheiden wir, was den ESP32-S3 wecken darf.
  // Wir schalten die "Dauerfeuer"-Interrupts aus:
  instance.sensor.disablePedometerIRQ();    // Schritte werden intern trotzdem gezählt
  instance.sensor.disableTiltIRQ();         // Kein Wecken beim Armdrehen
  instance.sensor.disableActivityIRQ();     // Kein Wecken bei Gehen/Laufen-Wechsel
  instance.sensor.disableAnyNoMotionIRQ();  // DAS spart den meisten Strom unterwegs!

  // Nur Wakeup (Double Tap) darf den Hardware-Pin triggern
  instance.sensor.enableWakeupIRQ();

  currentAccelleration.isMoved = true;
}
