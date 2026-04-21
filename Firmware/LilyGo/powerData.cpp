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
#include "powerData.h"

extern powerData currentPower;

powerData getPowerData() {
  Serial.println("getPowerData Start");

  currentPower.isCharging = instance.pmu.isCharging();
  currentPower.isDischarge = instance.pmu.isDischarge();
  currentPower.isUSBPlugin = instance.pmu.isVbusIn();
  currentPower.battVoltagemV = instance.pmu.getBattVoltage();
  currentPower.vBusVoltagemV = instance.pmu.getVbusVoltage();
  currentPower.systemVoltagemV = instance.pmu.getSystemVoltage();
  currentPower.batteryPercent = instance.pmu.getBatteryPercent();

  currentPower.code = CODE_NO_ERROR;

  Serial.print("getPowerData Battery Percent: "); Serial.println(currentPower.batteryPercent, DEC);

  return currentPower;
}
