#include <lvgl.h>
#include <LilyGoLib.h>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "dataCollection.h"
#include "alertData.h"
#include "settings.h"
#include "lora.h"

extern alertData currentAlerts;
extern singleAlert allAlerts[ALERT_MAX_NO];
extern lilygoSettings settings;

QueueHandle_t loraQueue;

void setupLora() {
  loraQueue = xQueueCreate(ALERT_MAX_NO, sizeof(LoraNotification));
}

void addMsgToLora(const char *msg) {
  LoraNotification loraMsg;
  strncpy(loraMsg.appName, "LilyGo", NAME_LEN);
  strncpy(loraMsg.title, "Messenger", TITLE_LEN);
  strncpy(loraMsg.body, msg, BODY_LEN);
  loraMsg.magic = settings.loraMagic;

  xQueueSend(loraQueue, &loraMsg, 0);  // Schiebt es in die Queue und läuft sofort weiter

  Serial.print("addMsgToLora Msg: ");
  Serial.println(msg);
}

void addAlertToLora(singleAlert alert) {
  LoraNotification loraMsg;
  strncpy(loraMsg.appName, alert.appName, NAME_LEN);
  strncpy(loraMsg.title, alert.title, TITLE_LEN);
  strncpy(loraMsg.body, alert.body, BODY_LEN);
  loraMsg.magic = settings.loraMagic;

  xQueueSend(loraQueue, &loraMsg, 0);  // Schiebt es in die Queue und läuft sofort weiter

  Serial.print("addAlertToLora ID: ");
  Serial.println(alert.id, DEC);
}

void processNewAlertsToLora(int oldMinIdx, int oldMaxIdx, int newMinIdx, int newMaxIdx) {
  // range: oldMaxIdx + 1 ... newMaxIdx
  for (int i = 0; i < currentAlerts.count; i++) {
    if ((allAlerts[i].id >= (oldMaxIdx + 1)) && (allAlerts[i].id <= newMaxIdx))
      addAlertToLora(allAlerts[i]);
  }
}

void transmitAlertsToLora() {
  LoraNotification loraMsg;

  UBaseType_t uxMessagesWaiting;
  // Check how many items are in the queue
  uxMessagesWaiting = uxQueueMessagesWaiting(loraQueue);

  if (uxMessagesWaiting > 0) {
    // Queue is not empty

    // 1. Radio aufwecken (falls RadioLib Standby/Sleep nutzt)
    instance.pmu.enableALDO4();  // Radio
    delay(10);
    radio.standby();
    settingLoRaParams();

    // send only 1 message per cycle (1 min)
    if (xQueueReceive(loraQueue, &loraMsg, 0) == pdPASS) {

      // 2. Paket zusammenbauen (z.B. als einfacher String oder Byte-Array)
      // String payload = String(loraMsg.appName) + ":" + loraMsg.title + ":" + loraMsg.body;

      // 3. Senden (Blockiert kurz während des Funkvorgangs)
      // int state = radio.transmit(payload);
      int state = radio.transmit((uint8_t*)&loraMsg, sizeof(loraMsg));

      Serial.print("transmitAlertsToLora transmit state: ");
      Serial.println(state, DEC);
    }

    // 4. Radio sofort wieder in den Deep Sleep
    radio.sleep();
    instance.pmu.disableALDO4();  // Radio
  }
}

void settingLoRaParams() {
  // set carrier frequency
  if (radio.setFrequency(868.0) == RADIOLIB_ERR_INVALID_FREQUENCY) {
    Serial.println(F("Selected frequency is invalid for this module!"));
  }

  // set bandwidth (original 125 kHz)
  if (radio.setBandwidth(125.0) == RADIOLIB_ERR_INVALID_BANDWIDTH) {
    Serial.println(F("Selected bandwidth is invalid for this module!"));
  }

  // set spreading factor (original 10)
  if (radio.setSpreadingFactor(9) == RADIOLIB_ERR_INVALID_SPREADING_FACTOR) {
    Serial.println(F("Selected spreading factor is invalid for this module!"));
  }

  // set coding rate (original 6)
  if (radio.setCodingRate(7) == RADIOLIB_ERR_INVALID_CODING_RATE) {
    Serial.println(F("Selected coding rate is invalid for this module!"));
  }

  // set LoRa sync word (original 0xAB)
  if (radio.setSyncWord(0x12) != RADIOLIB_ERR_NONE) {
    Serial.println(F("Unable to set sync word!"));
  }

  // set output power (original 22 dBm) (accepted range is -17 - 22 dBm)
  if (radio.setOutputPower(13) == RADIOLIB_ERR_INVALID_OUTPUT_POWER) {
    Serial.println(F("Selected output power is invalid for this module!"));
  }

  // set over current protection limit (original 140 mA) (accepted range is 45 - 240 mA)
  // NOTE: set value to 0 to disable overcurrent protection
  if (radio.setCurrentLimit(60) == RADIOLIB_ERR_INVALID_CURRENT_LIMIT) {
    Serial.println(F("Selected current limit is invalid for this module!"));
  }

  // set LoRa preamble length (original 15 symbols) (accepted range is 0 - 65535)
  if (radio.setPreambleLength(8) == RADIOLIB_ERR_INVALID_PREAMBLE_LENGTH) {
    Serial.println(F("Selected preamble length is invalid for this module!"));
  }

  // disable CRC
  if (radio.setCRC(false) == RADIOLIB_ERR_INVALID_CRC_CONFIGURATION) {
    Serial.println(F("Selected CRC is invalid for this module!"));
  }

  // Set TCXO voltage (original 3.0V)
  if (radio.setTCXO(1.6) == RADIOLIB_ERR_INVALID_TCXO_VOLTAGE) {
    Serial.println(F("Selected TCXO voltage is invalid for this module!"));
  }

  // Set use DIO2 as RF switch.
  if (radio.setDio2AsRfSwitch() != RADIOLIB_ERR_NONE) {
    Serial.println(F("Failed to set DIO2 as RF switch!"));
  }
}
