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
#include <WiFiUdp.h>

#include "syncNTP.h"

WiFiUDP ntpUDP;

void syncNTP(long gmt, const char *ntpServer) {
  struct tm hwTimeinfo;

  Serial.println("syncNTP Start");

  // hardware clock
  instance.rtc.getDateTime(&hwTimeinfo);
  Serial.print("syncNTP hardware clock:");
  Serial.println(&hwTimeinfo, "%A, %B %d %Y %H:%M:%S");

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("syncNTP uses ntpServer ");
    Serial.println(ntpServer);

    // NTPClient(udp, server, offset_in_seconds, update_interval_in_ms)
    NTPClient timeClient(ntpUDP, ntpServer, gmt);  // 3600 = GMT+1

    timeClient.begin();
    if (timeClient.update()) {
      Serial.println("NTP Sync erfolgreich!");
      unsigned long epochTime = timeClient.getEpochTime();

      // Jetzt die interne ESP32-Zeit (RTC) setzen
      struct timeval tv;
      tv.tv_sec = epochTime;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
    }
    timeClient.end();
  }

  // system clock
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("syncNTP No time available (yet)");
  } else {
    Serial.print("syncNTP system clock: ");
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  }
}
