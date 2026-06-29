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

#include "dataCollection.h"
#include "alertData.h"
#include "config.h"
#include "settings.h"
#include "powerData.h"
#include "timerEvent.h"
#include "deviceEvent.h"
#include "lora.h"

extern alertData currentAlerts;
extern singleAlert allAlerts[ALERT_MAX_NO];
extern powerData currentPower;
extern int guiState;


alertData getAlertData(const String gatewayIP, const String macAdress) {
  JSONVar alerts;

  Serial.println("getAlertData Start");

  currentAlerts.code = CODE_NO_ERROR;
  HTTPClient http;               // Use location API if WiFi is connected
  http.setConnectTimeout(3000);  // 3 second max timeout
  String locationQueryURL = "http://" + gatewayIP + ":8080/alert?MAC=" + macAdress;
  http.begin(locationQueryURL.c_str());
  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    int oldNo = currentAlerts.count;
    int oldMin = 0;
    int oldMax = 0;
    if (oldNo > 0) {
      oldMin = allAlerts[0].id;
      oldMax = allAlerts[oldNo - 1].id;
    }

    String payload = http.getString();
    if (payload.length() > 16 * 1024) {
      currentAlerts.code = CODE_PARSE_ERROR;
      strncpy(currentAlerts.log, "Invalid payload", LOG_LEN - 1);
      Serial.print("getAlertData Invalid payload: ");
      Serial.println(payload.length(), DEC);
      return currentAlerts;
    }

    alerts = JSON.parse(payload);
    if (!alerts.hasOwnProperty("data") || JSON.typeof(alerts["data"]) != "array") {
      currentAlerts.code = CODE_PARSE_ERROR;
      Serial.println("getAlertData Parsing error");
      return currentAlerts;
    }

    int alertNo = alerts["data"].length();
    if (alertNo > ALERT_MAX_NO) {
      alertNo = ALERT_MAX_NO;
    }
    currentAlerts.count = alertNo;

    for (int i = 0; i < currentAlerts.count; i++) {
      JSONVar alert = alerts["data"][i];

      String _string = (const char *)alert["appName"];
      strncpy(allAlerts[i].appName, _string.c_str(), NAME_LEN - 1);
      allAlerts[i].appName[NAME_LEN - 1] = '\0';

      _string = (const char *)alert["title"];
      if (_string == "null")
        _string = "";
      _string = cleanNotificationText(_string);
      strncpy(allAlerts[i].title, _string.c_str(), TITLE_LEN - 1);
      allAlerts[i].title[TITLE_LEN - 1] = '\0';

      _string = (const char *)alert["body"];
      if (_string == nullptr) _string = "";

      if ((_string != null) && (_string.length() > 0)) {
        if (_string == "null")
          _string = "";
        _string = cleanNotificationText(_string);
        strncpy(allAlerts[i].body, _string.c_str(), BODY_LEN - 1);
        allAlerts[i].body[BODY_LEN - 1] = '\0';
      } else {
        strcpy(allAlerts[i].body, "");
      }

      _string = (const char *)alert["dismissed"];
      if (_string == "false") {
        allAlerts[i].dismissed = false;
      } else
        allAlerts[i].dismissed = true;

      _string = (const char *)(alert["timestamp"]);
      _string.replace("T", " ");
      int index = _string.lastIndexOf(".");
      if (index < 0) index = _string.length();
      _string = _string.substring(0, index);
      strncpy(allAlerts[i].timeStamp, _string.c_str(), sizeof(allAlerts[i].timeStamp) - 1);
      allAlerts[i].timeStamp[sizeof(allAlerts[i].timeStamp) - 1] = '\0';

      allAlerts[i].id = int(alert["id"]);
    }

    int newNo = currentAlerts.count;
    int newMin = 0;
    int newMax = 0;
    if (newNo > 0) {
      newMin = allAlerts[0].id;
      newMax = allAlerts[newNo - 1].id;

      String log = String(newMin) + " " + String(newMax) + " " + String(oldMin) + " " + String(oldMax);
      strncpy(currentAlerts.log, log.c_str(), sizeof(currentAlerts.log) - 1);
      currentAlerts.log[sizeof(currentAlerts.log) - 1] = '\0';

      if ((oldNo != newNo) || (oldMin != newMin) || (oldMax != newMax)) {
        if (currentPower.batteryPercent > 10) {
          vibMotor();
        }
        if (guiState == DARK_STATE) {
          // GUI state
          guiState = ALERT_STATE;

          // prepare the  screen object
          prepareAlertScreen();

          // show the newest alert
          int alertIndex = currentAlerts.count - 1;
          showAlert(allAlerts[alertIndex], alertIndex, 0);

          // set brightness
          displayWakup();
          startBrightnessTimer(BRIGHTNESS_TIMEOUT_ALERT);
        }
        // process to Lora
        processNewAlertsToLora(oldMin, oldMax, newMin, newMax);
      }
    }

    Serial.print("getAlertData No. of alerts: ");
    Serial.print(currentAlerts.count, DEC);
    Serial.print(" Min. ID: ");
    Serial.print(newMin, DEC);
    Serial.print(" Max. ID: ");
    Serial.println(newMax, DEC);
  } else {
    // http error
    currentAlerts.code = CODE_HTTP_ERROR;

    Serial.print("getAlertData Error code: ");
    Serial.println(currentAlerts.code, DEC);
  }
  strncpy(currentAlerts.log, String(httpResponseCode).c_str(), sizeof(currentAlerts.log) - 1);
  currentAlerts.log[sizeof(currentAlerts.log) - 1] = '\0';

  http.end();
  return currentAlerts;
}

void vibMotor() {
  // from firmware src
  instance.drv.selectLibrary(1);
  instance.drv.setMode(SensorDRV2605::MODE_INTTRIG);
  instance.drv.useERM();
  // set wave
  // Wir nutzen "Sharp Tick" (ID 4) oder "Strong Buzz" (ID 47)
  // Aber wir lassen die Pausen (134) weg, um die Trägheit zu überwinden
  instance.drv.setWaveform(0, 47);  // Buzz 100% (lang)
  instance.drv.setWaveform(1, 47);  // Direkt nochmal ohne Pause
  instance.drv.setWaveform(2, 47);  // Und ein drittes Mal
  instance.drv.setWaveform(3, 12);  // Triple Click als "Abschluss-Rüttler"
  instance.drv.setWaveform(4, 0);   // Ende
  // ID Effekt-Name Gefühl
  // 1  Strong Click Kräftiges Bestätigen
  // 7  Soft Bump Dezenter Hinweis
  // 12 Triple Click  Alarm oder kritischer Fehler
  // 14 Soft Fuzz Leichtes Zittern
  // 47 Buzz 100% Klassischer Vibrationsalarm (lang)
  // 51 Transition Hum  Sanftes Ansteigen
  // 58 Long Buzz Für Anrufe / Wecker
  // play the effect
  instance.drv.run();
}

String cleanNotificationText(String source) {
  // === BEREICH \u00c0 bis \u00ff (Lateinisch-1, Ergänzung) ===
  source.replace("\xC3\x80", "A");   // \u00c0
  source.replace("\xC3\x81", "A");   // \u00c1
  source.replace("\xC3\x82", "A");   // \u00c2
  source.replace("\xC3\x83", "A");   // \u00c3
  source.replace("\xC3\x84", "Ae");  // \u00c4 (Oder "Ae", falls gewünscht)
  source.replace("\xC3\x85", "A");   // \u00c5
  source.replace("\xC3\x86", "A");   // \u00c6
  source.replace("\xC3\x87", "C");   // \u00c7
  source.replace("\xC3\x88", "E");   // \u00c8
  source.replace("\xC3\x89", "E");   // \u00c9
  source.replace("\xC3\x8a", "E");   // \u00ca
  source.replace("\xC3\x8b", "E");   // \u00cb
  source.replace("\xC3\x8c", "I");   // \u00cc
  source.replace("\xC3\x8d", "I");   // \u00cd
  source.replace("\xC3\x8e", "I");   // \u00ce
  source.replace("\xC3\x8f", "I");   // \u00cf
  source.replace("\xC3\x90", "D");   // \u00d0
  source.replace("\xC3\x91", "N");   // \u00d1
  source.replace("\xC3\x92", "O");   // \u00d2
  source.replace("\xC3\x93", "O");   // \u00d3
  source.replace("\xC3\x94", "O");   // \u00d4
  source.replace("\xC3\x95", "O");   // \u00d5
  source.replace("\xC3\x96", "Oe");  // \u00d6
  source.replace("\xC3\x97", "x");   // \u00d7
  source.replace("\xC3\x98", "O");   // \u00d8
  source.replace("\xC3\x99", "U");   // \u00d9
  source.replace("\xC3\x9a", "U");   // \u00da
  source.replace("\xC3\x9b", "U");   // \u00db
  source.replace("\xC3\x9c", "Ue");  // \u00dc
  source.replace("\xC3\x9d", "Y");   // \u00dd
  source.replace("\xC3\x9f", "ss");  // \u00df
  source.replace("\xC3\xa0", "a");   // \u00e0
  source.replace("\xC3\xa1", "a");   // \u00e1
  source.replace("\xC3\xa2", "a");   // \u00e2
  source.replace("\xC3\xa3", "a");   // \u00e3
  source.replace("\xC3\xa4", "ae");  // \u00e4
  source.replace("\xC3\xa5", "a");   // \u00e5
  source.replace("\xC3\xa6", "ae");  // \u00e6
  source.replace("\xC3\xa7", "c");   // \u00e7
  source.replace("\xC3\xa8", "e");   // \u00e8
  source.replace("\xC3\xa9", "e");   // \u00e9
  source.replace("\xC3\xaa", "e");   // \u00ea
  source.replace("\xC3\xab", "e");   // \u00eb
  source.replace("\xC3\xac", "i");   // \u00ec
  source.replace("\xC3\xad", "i");   // \u00ed
  source.replace("\xC3\xae", "i");   // \u00ee
  source.replace("\xC3\xaf", "i");   // \u00ef
  source.replace("\xC3\xb0", "o");   // \u00f0
  source.replace("\xC3\xb1", "n");   // \u00f1
  source.replace("\xC3\xb2", "o");   // \u00f2
  source.replace("\xC3\xb3", "o");   // \u00f3
  source.replace("\xC3\xb4", "o");   // \u00f4
  source.replace("\xC3\xb5", "o");   // \u00f5
  source.replace("\xC3\xb6", "oe");  // \u00f6
  source.replace("\xC3\xb8", "o");   // \u00f8
  source.replace("\xC3\xb9", "u");   // \u00f9
  source.replace("\xC3\xba", "u");   // \u00fa
  source.replace("\xC3\xbb", "u");   // \u00fb
  source.replace("\xC3\xbc", "ue");  // \u00fc
  source.replace("\xC3\xbd", "y");   // \u00fd
  source.replace("\xC3\xbf", "y");   // \u00ff

  // === BEREICH \u0100 bis \u017e (Lateinisch, Erweitert-A) ===
  source.replace("\xC4\x80", "A");   // \u0100
  source.replace("\xC4\x81", "a");   // \u0101
  source.replace("\xC4\x82", "A");   // \u0102
  source.replace("\xC4\x83", "a");   // \u0103
  source.replace("\xC4\x84", "A");   // \u0104
  source.replace("\xC4\x85", "a");   // \u0105
  source.replace("\xC4\x86", "C");   // \u0106
  source.replace("\xC4\x87", "c");   // \u0107
  source.replace("\xC4\x88", "C");   // \u0108
  source.replace("\xC4\x89", "c");   // \u0109
  source.replace("\xC4\x8a", "C");   // \u010a
  source.replace("\xC4\x8b", "c");   // \u010b
  source.replace("\xC4\x8c", "C");   // \u010c
  source.replace("\xC4\x8d", "c");   // \u010d
  source.replace("\xC4\x8e", "D");   // \u010e
  source.replace("\xC4\x8f", "d");   // \u010f
  source.replace("\xC4\x90", "D");   // \u0110
  source.replace("\xC4\x91", "d");   // \u0111
  source.replace("\xC4\x92", "E");   // \u0112
  source.replace("\xC4\x93", "e");   // \u0113
  source.replace("\xC4\x94", "E");   // \u0114
  source.replace("\xC4\x95", "e");   // \u0115
  source.replace("\xC4\x96", "E");   // \u0116
  source.replace("\xC4\x97", "e");   // \u0117
  source.replace("\xC4\x98", "E");   // \u0118
  source.replace("\xC4\x99", "e");   // \u0119
  source.replace("\xC4\x9a", "E");   // \u011a
  source.replace("\xC4\x9b", "e");   // \u011b
  source.replace("\xC4\x9c", "G");   // \u011c
  source.replace("\xC4\x9d", "g");   // \u011d
  source.replace("\xC4\x9e", "G");   // \u011e
  source.replace("\xC4\x9f", "g");   // \u011f
  source.replace("\xC4\xa0", "G");   // \u0120
  source.replace("\xC4\xa1", "g");   // \u0121
  source.replace("\xC4\xa2", "G");   // \u0122
  source.replace("\xC4\xa3", "g");   // \u0123
  source.replace("\xC4\xa4", "H");   // \u0124
  source.replace("\xC4\xa5", "h");   // \u0125
  source.replace("\xC4\xa6", "H");   // \u0126
  source.replace("\xC4\xa7", "h");   // \u0127
  source.replace("\xC4\xa8", "I");   // \u0128
  source.replace("\xC4\xa9", "i");   // \u0129
  source.replace("\xC4\xaa", "I");   // \u012a
  source.replace("\xC4\xab", "i");   // \u012b
  source.replace("\xC4\xac", "I");   // \u012c
  source.replace("\xC4\xad", "i");   // \u012d
  source.replace("\xC4\xae", "I");   // \u012e
  source.replace("\xC4\xaf", "i");   // \u012f
  source.replace("\xC4\xb0", "I");   // \u0130
  source.replace("\xC4\xb1", "i");   // \u0131
  source.replace("\xC4\xb2", "IJ");  // \u0132
  source.replace("\xC4\xb3", "ij");  // \u0133
  source.replace("\xC4\xb4", "J");   // \u0134
  source.replace("\xC4\xb5", "j");   // \u0135
  source.replace("\xC4\xb6", "K");   // \u0136
  source.replace("\xC4\xb7", "k");   // \u0137
  source.replace("\xC4\xb8", "k");   // \u0138
  source.replace("\xC4\xb9", "L");   // \u0139
  source.replace("\xC4\xba", "l");   // \u013a
  source.replace("\xC4\xbb", "L");   // \u013b
  source.replace("\xC4\xbc", "l");   // \u013c
  source.replace("\xC4\xbd", "L");   // \u013d
  source.replace("\xC4\xbe", "l");   // \u013e
  source.replace("\xC4\xbf", "L");   // \u013f
  source.replace("\xC5\x80", "l");   // \u0140
  source.replace("\xC5\x81", "L");   // \u0141
  source.replace("\xC5\x82", "l");   // \u0142
  source.replace("\xC5\x83", "N");   // \u0143
  source.replace("\xC5\x84", "n");   // \u0144
  source.replace("\xC5\x85", "N");   // \u0145
  source.replace("\xC5\x86", "n");   // \u0146
  source.replace("\xC5\x87", "N");   // \u0147
  source.replace("\xC5\x88", "n");   // \u0148
  source.replace("\xC5\x8c", "O");   // \u014c
  source.replace("\xC5\x8d", "o");   // \u014d
  source.replace("\xC5\x8e", "O");   // \u014e
  source.replace("\xC5\x8f", "o");   // \u014f
  source.replace("\xC5\x90", "O");   // \u0150
  source.replace("\xC5\x91", "o");   // \u0151
  source.replace("\xC5\x92", "OE");  // \u0152
  source.replace("\xC5\x93", "oe");  // \u0153
  source.replace("\xC5\x94", "R");   // \u0154
  source.replace("\xC5\x95", "r");   // \u0155
  source.replace("\xC5\x96", "R");   // \u0156
  source.replace("\xC5\x97", "r");   // \u0157
  source.replace("\xC5\x98", "R");   // \u0158
  source.replace("\xC5\x99", "r");   // \u0159
  source.replace("\xC5\x9a", "S");   // \u015a
  source.replace("\xC5\x9b", "s");   // \u015b
  source.replace("\xC5\x9c", "S");   // \u015c
  source.replace("\xC5\x9d", "s");   // \u015d
  source.replace("\xC5\x9e", "S");   // \u015e
  source.replace("\xC5\x9f", "s");   // \u015f
  source.replace("\xC5\xa0", "S");   // \u0160
  source.replace("\xC5\xa1", "s");   // \u0161
  source.replace("\xC5\xa2", "T");   // \u0162
  source.replace("\xC5\xa3", "t");   // \u0163
  source.replace("\xC5\xa4", "T");   // \u0164
  source.replace("\xC5\xa5", "t");   // \u0165
  source.replace("\xC5\xa6", "T");   // \u0166
  source.replace("\xC5\xa7", "t");   // \u0167
  source.replace("\xC5\xa8", "U");   // \u0168
  source.replace("\xC5\xa9", "u");   // \u0169
  source.replace("\xC5\xaa", "U");   // \u016a
  source.replace("\xC5\xab", "u");   // \u016b
  source.replace("\xC5\xac", "U");   // \u016c
  source.replace("\xC5\xad", "u");   // \u016d
  source.replace("\xC5\xae", "U");   // \u016e
  source.replace("\xC5\xaf", "u");   // \u016f
  source.replace("\xC5\xb0", "U");   // \u0170
  source.replace("\xC5\xb1", "u");   // \u0171
  source.replace("\xC5\xb2", "U");   // \u0172
  source.replace("\xC5\xb3", "u");   // \u0173
  source.replace("\xC5\xb4", "W");   // \u0174
  source.replace("\xC5\xb5", "w");   // \u0175
  source.replace("\xC5\xb6", "Y");   // \u0176
  source.replace("\xC5\xb7", "y");   // \u0177
  source.replace("\xC5\xb8", "Y");   // \u0178
  source.replace("\xC5\xb9", "Z");   // \u0179
  source.replace("\xC5\xba", "z");   // \u017a
  source.replace("\xC5\xbb", "Z");   // \u017b
  source.replace("\xC5\xbc", "z");   // \u017c
  source.replace("\xC5\xbd", "Z");   // \u017d
  source.replace("\xC5\xbe", "z");   // \u017e

  // Erwähnte Korrekturen & Ergänzungen für Westeuropäisch (Accents)
  source.replace("\xC3\xA8", "e");  // \u00e8 -> è (Das hat gefehlt!)
  source.replace("\xC3\xAC", "i");  // \u00ec -> ì
  source.replace("\xC3\xB2", "o");  // \u00f2 -> ò
  source.replace("\xC3\xB9", "u");  // \u00f9 -> ù

  // Große Gegenstücke zu den Akzenten (falls jemand SCHREIT)
  source.replace("\xC3\x88", "E");  // \u00c8 -> È
  source.replace("\xC3\x89", "E");  // \u00c9 -> É
  source.replace("\xC3\x8A", "E");  // \u00ca -> Ê
  source.replace("\xC3\x8B", "E");  // \u00cb -> Ë

  // Geschütztes Leerzeichen (NBSP) durch ein stinknormales Leerzeichen ersetzen
  source.replace("\xC2\xA0", " ");
  // Schmales geschütztes Leerzeichen ebenfalls ersetzen
  source.replace("\xE2\x80\xAF", " ");

  return source;
}