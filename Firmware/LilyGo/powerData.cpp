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
#include <Preferences.h>

#include "config.h"
#include "dataCollection.h"
#include "powerData.h"

extern powerData currentPower;
extern uint8_t batteryCapacityHistory[1440];
extern uint16_t batteryVoltageHistory[1440];

// Die exakten Eckpunkte aus deinem Diagramm
// const uint16_t maxVolt = 4330;  // 100%
// const uint16_t minVolt = 3550;  // 0%
const uint16_t maxVolt = 4300;  // 100%
const uint16_t minVolt = 3680;  // 0%

void setupPowerMgt() {
  // Clear all interrupt status
  instance.pmu.clearIrqStatus();

  // Enable the required interrupt function
  instance.pmu.enableIRQ(
    XPOWERS_AXP2101_BAT_INSERT_IRQ | XPOWERS_AXP2101_BAT_REMOVE_IRQ |     // BATTERY
    XPOWERS_AXP2101_VBUS_INSERT_IRQ | XPOWERS_AXP2101_VBUS_REMOVE_IRQ |   // VBUS
    XPOWERS_AXP2101_PKEY_SHORT_IRQ | XPOWERS_AXP2101_PKEY_LONG_IRQ |      // POWER KEY
    XPOWERS_AXP2101_BAT_CHG_DONE_IRQ | XPOWERS_AXP2101_BAT_CHG_START_IRQ  // CHARGE
  );

  //Enable or Disable PMU Feature
  instance.pmu.enableBattDetection();
  instance.pmu.enableVbusVoltageMeasure();
  instance.pmu.enableBattVoltageMeasure();
  instance.pmu.enableSystemVoltageMeasure();

  instance.pmu.disableDC3();    // GPS aus
  instance.pmu.disableBLDO1();  // GPS aus
  instance.pmu.disableALDO4();  // Radio (Falls du nur BLE nutzt, prüfe ob das nötig ist. Oft ist das die Versorgung für externe Funkmodule)

  // CPU clock
  setCpuFrequencyMhz(80);

  // alle Kommunikationskanäle stoppen
  btStop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  // Battery history
  for (int i = 0; i < 1440; i++) {
    batteryCapacityHistory[i] = 0;
    batteryVoltageHistory[i] = 0;
  }
}

powerData getPowerData() {
  Serial.println("getPowerData Start");

  currentPower.isCharging = instance.pmu.isCharging();
  currentPower.isDischarge = instance.pmu.isDischarge();
  currentPower.isUSBPlugin = instance.pmu.isVbusIn();
  currentPower.battVoltagemV = instance.pmu.getBattVoltage();
  currentPower.vBusVoltagemV = instance.pmu.getVbusVoltage();
  currentPower.systemVoltagemV = instance.pmu.getSystemVoltage();
  // currentPower.batteryPercent = instance.pmu.getBatteryPercent();
  currentPower.batteryPercent = getCustomBatteryPercent(currentPower.battVoltagemV);

  currentPower.code = CODE_NO_ERROR;

  Serial.print("getPowerData Battery Percent: ");
  Serial.print(currentPower.batteryPercent, DEC);
  Serial.print(", Battery voltage: ");
  Serial.println(currentPower.battVoltagemV, DEC);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    // reset battery history at midnight
    if (timeinfo.tm_hour == 0 && timeinfo.tm_min == 0) {
      for (int i = 0; i < 1440; i++) {
        batteryCapacityHistory[i] = 0;
        batteryVoltageHistory[i] = 0;
      }
    }

    // store current battery capacity value
    int currentMinuteIndex = 0;
    currentMinuteIndex = timeinfo.tm_hour * 60 + timeinfo.tm_min;
    batteryCapacityHistory[currentMinuteIndex] = currentPower.batteryPercent;
    batteryVoltageHistory[currentMinuteIndex] = currentPower.battVoltagemV;
  }

  return currentPower;
}

void verifyPowerMgt() {
  Serial.printf("DC3 Enabled: %s\n", instance.pmu.isEnableDC3() ? "YES" : "NO");
  Serial.printf("BLDO1 Enabled: %s\n", instance.pmu.isEnableBLDO1() ? "YES" : "NO");
  Serial.printf("ALDO4 Enabled: %s\n", instance.pmu.isEnableALDO4() ? "YES" : "NO");
  uint32_t freq = getCpuFrequencyMhz();
  Serial.printf("CPU Frequency: %u MHz\n", freq);
}

void resetBatteryCalibration() {
  // 0. include this function call in setup() directly before instance.begin()
  // 1. flash firmware including this function call
  // 2. load the battery to 100% (ensure loaduing current close to 0)
  // 3. flash firmware excluding this funtion call
  // 4. run the T-Watch down to 0 battery capacity without any interruption (switch-off, re-flash, etc.)

  // =================================================================
  // KALIBRIERUNGS-RESET (BEVOR DIE LIBRARY STARTET)
  // =================================================================
  delay(500);
  Preferences prefs;
  prefs.begin("lilygo", false);

  Serial.println("Lösche altes Kalibrierungs-Flag im Flash...");
  // Wir setzen es hart auf false, damit instance.begin() gleich anspringt
  prefs.putBool("calibration", false);
  prefs.end();

  // Now start the library - it will see 'calibration = false'
  // and trigger the internal 470mAh calibration automatically!
  // in setup(): instance.begin();
}

uint8_t getCustomBatteryPercent(uint16_t mv) {
  // Sicherheits-Kappen für Ausreißer
  if (mv >= maxVolt) return 100;
  if (mv <= minVolt) return 0;

  // Lineares Mapping: Rechnet den Bereich [3550...4330] sauber auf [0...100] um
  return (uint8_t)(((mv - minVolt) * 100) / (maxVolt - minVolt));
}
