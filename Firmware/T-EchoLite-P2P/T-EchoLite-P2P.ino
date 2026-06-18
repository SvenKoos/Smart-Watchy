#include <Arduino.h>
#include "Adafruit_EPD.h"
#include "RadioLib.h"
#include "t_echo_lite_config.h"
#include "Display_Fonts.h"

#include <Crypto.h>
#include <AES.h>
#include <CTR.h>

#define NAME_LEN 32
#define TITLE_LEN 64
#define BODY_LEN 128

typedef struct {
  uint32_t magic;
  char appName[NAME_LEN];
  char title[TITLE_LEN];
  char body[BODY_LEN];
} EncryptedPayload;

typedef struct {
  uint32_t packetCounter;  // Bleibt unverschlüsselt (wird für den IV benötigt)
  EncryptedPayload data;   // Wird verschlüsselt
} LoraNotification;

CTR<AES128> ctraes;

// encryption
// 16-Byte Schlüssel (128-Bit), den T-Watch und T-Echo teilen
const byte key[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F };

static const uint32_t Local_MAC[2] = {
  NRF_FICR->DEVICEID[0],
  NRF_FICR->DEVICEID[1],
};

struct Display_Refresh_Operator {
  struct
  {
    bool transmission_fast_refresh_flag = false;
  } sx1262_test;
};

struct SX1262_Operator {
  using mode = enum {
    LORA,  // lora mode
    FSK,   // fsk mode
  };

  // aligned with T-Watch
  struct
  {
    float value = 868.1;
    bool change_flag = false;
  } frequency;
  struct
  {
    float value = 125.0;
    bool change_flag = false;
  } bandwidth;
  struct
  {
    // uint8_t value = 12;
    uint8_t value = 9;
    bool change_flag = false;
  } spreading_factor;
  struct
  {
    // uint8_t value = 8;
    uint8_t value = 7;
    bool change_flag = false;
  } coding_rate;
  struct
  {
    // uint8_t value = 0xAB;
    uint8_t value = 0x12;
    bool change_flag = false;
  } sync_word;
  struct
  {
    // int8_t value = 22;
    int8_t value = 13;
    bool change_flag = false;
  } output_power;
  struct
  {
    // float value = 140;
    float value = 60;
    bool change_flag = false;
  } current_limit;
  struct
  {
    // int16_t value = 16;
    int16_t value = 8;
    bool change_flag = false;
  } preamble_length;
  struct
  {
    bool value = false;
    bool change_flag = false;
  } crc;

  uint8_t current_mode = mode::LORA;

  volatile bool operation_flag = false;
  bool initialization_flag = false;

  float receive_rssi = 0;
  float receive_snr = 0;
};

SX1262_Operator SX1262_OP;
Display_Refresh_Operator Display_Refresh_OP;

SPIClass Custom_SPI_0(NRF_SPIM0, SCREEN_MISO, SCREEN_SCLK, SCREEN_MOSI);
Adafruit_SSD1681 display(SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_DC, SCREEN_RST,
                         SCREEN_CS, SCREEN_SRAM_CS, SCREEN_BUSY, &Custom_SPI_0);

SPIClass Custom_SPI_3(NRF_SPIM3, SX1262_MISO, SX1262_SCLK, SX1262_MOSI);
SX1262 radio = new Module(SX1262_CS, SX1262_DIO1, SX1262_RST, SX1262_BUSY, Custom_SPI_3);

void Dio1_Action_Interrupt(void) {
  // we sent or received a packet, set the flag
  SX1262_OP.operation_flag = true;
}

void Set_SX1262_RF_Transmitter_Switch(bool status) {
  if (status == true) {
    digitalWrite(SX1262_RF_VC1, HIGH);  // send
    digitalWrite(SX1262_RF_VC2, LOW);
  } else {
    digitalWrite(SX1262_RF_VC1, LOW);  // receive
    digitalWrite(SX1262_RF_VC2, HIGH);
  }
}

bool SX1262_Set_Default_Parameters(String *assertion) {
  if (radio.setFrequency(SX1262_OP.frequency.value) != RADIOLIB_ERR_NONE) {
    *assertion = "Failed to set frequency value";
    return false;
  }
  if (radio.setBandwidth(SX1262_OP.bandwidth.value) != RADIOLIB_ERR_NONE) {
    *assertion = "Failed to set bandwidth value";
    return false;
  }
  if (radio.setOutputPower(SX1262_OP.output_power.value) != RADIOLIB_ERR_NONE) {
    *assertion = "Failed to set output_power value";
    return false;
  }
  if (radio.setCurrentLimit(SX1262_OP.current_limit.value) != RADIOLIB_ERR_NONE) {
    *assertion = "Failed to set current_limit value";
    return false;
  }
  if (radio.setPreambleLength(SX1262_OP.preamble_length.value) != RADIOLIB_ERR_NONE) {
    *assertion = "Failed to set preamble_length value";
    return false;
  }
  if (radio.setCRC(SX1262_OP.crc.value) != RADIOLIB_ERR_NONE) {
    *assertion = "Failed to set crc value";
    return false;
  }
  if (SX1262_OP.current_mode == SX1262_OP.mode::LORA) {
    if (radio.setSpreadingFactor(SX1262_OP.spreading_factor.value) != RADIOLIB_ERR_NONE) {
      *assertion = "Failed to set spreading_factor value";
      return false;
    }
    if (radio.setCodingRate(SX1262_OP.coding_rate.value) != RADIOLIB_ERR_NONE) {
      *assertion = "Failed to set coding_rate value";
      return false;
    }
    if (radio.setSyncWord(SX1262_OP.sync_word.value) != RADIOLIB_ERR_NONE) {
      *assertion = "Failed to set sync_word value";
      return false;
    }
  } else {
  }
  return true;
}

void GFX_Print_SX1262_Info(void) {
  display.fillScreen(EPD_WHITE);

  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(5, 20);
  display.setTextSize(1);

  display.printf("SX1262 Info");

  display.setFont(&FreeSans9pt7b);

  display.setCursor(5, 40);
  display.printf("MAC 0: %u", Local_MAC[0]);
  display.setCursor(5, 60);
  display.printf("MAC 1: %u", Local_MAC[1]);
}

void GFX_Print_SX1262_Init_Successful_Refresh_Info(void) {
  display.setTextSize(1);
  display.setCursor(5, 80);
  display.printf("Status: Init successful");

  display.setCursor(5, 100);
  if (SX1262_OP.current_mode == SX1262_OP.mode::LORA) {
    display.printf("Mode: LoRa");
  } else {
    display.printf("Mode: FSK");
  }

  display.setCursor(5, 120);
  display.printf("Frequency: %.1f MHz", SX1262_OP.frequency.value);

  display.setCursor(5, 140);
  display.printf("Bandwidth: %.1f KHz", SX1262_OP.bandwidth.value);

  display.setCursor(5, 160);
  display.printf("Output Power: %d dBm", SX1262_OP.output_power.value);
}

void GFX_Print_SX1262_Init_Failed_Refresh_Info(void) {
  display.setTextSize(1);
  display.setCursor(5, 80);
  display.printf("Status: Init failed");
}

// Funktion zur Konvertierung von Unix-Time in hh:mm (Lokale Zeit)
String getFormattedTime(uint32_t unixTime) {
  // 1. Zeitzone anpassen (z.B. Hamburg / Mitteleuropäische Zeit)
  // MEZ = UTC + 1 Stunde (3600 Sek) | MESZ (Sommerzeit) = UTC + 2 Stunden (7200 Sek)
  // Da wir aktuell Mai 2026 haben, befinden wir uns in der Sommerzeit (MESZ):
  // ist bereits angepasst bei Sender
  uint32_t localTime = unixTime;

  // 2. Sekunden des aktuellen Tages isolieren
  uint32_t secondsInDay = localTime % 86400;  // 86400 Sekunden hat ein Tag

  // 3. Stunden und Minuten berechnen
  uint8_t hours = secondsInDay / 3600;
  uint8_t minutes = (secondsInDay % 3600) / 60;

  // 4. Als sauber formatierten String zurückgeben (mit führenden Nullen)
  char timeBuffer[6];  // Platz für "hh:mm\0"
  sprintf(timeBuffer, "%02d:%02d", hours, minutes);

  return String(timeBuffer);
}

void GFX_Print_SX1262_Info_Loop(void) {
  if (SX1262_OP.initialization_flag == true) {
    if (Display_Refresh_OP.sx1262_test.transmission_fast_refresh_flag == true) {
      Display_Refresh_OP.sx1262_test.transmission_fast_refresh_flag = false;
      // display.display(display.Update_Mode::FAST_REFRESH, true);
      display.display();
    }

    if (SX1262_OP.operation_flag == true) {
      SX1262_OP.operation_flag = false;

      LoraNotification receivedMsg;

      // Bytes direkt aus dem SX1262 in dein Krypto-Struct einlesen
      int state = radio.readData((uint8_t *)&receivedMsg, sizeof(LoraNotification));

      if (state == RADIOLIB_ERR_NONE) {
        Serial.println("[Krypto] Paket empfangen! Starte Entschlüsselung...");

        // 1. IV aus dem unverschlüsselten packetCounter (Zeitstempel) rekonstruieren
        byte iv[16] = { 0 };
        memcpy(iv, &(receivedMsg.packetCounter), sizeof(receivedMsg.packetCounter));

        // 2. Krypto-Engine vorbereiten
        ctraes.setKey(key, sizeof(key));
        ctraes.setIV(iv, sizeof(iv));

        // 3. Den verschlüsselten data-Teil "In-Place" entschlüsseln
        ctraes.decrypt((byte *)&(receivedMsg.data), (byte *)&(receivedMsg.data), sizeof(EncryptedPayload));

        // 4. Sicherheitscheck: Stimmt das Magic Word?
        if (receivedMsg.data.magic == 0xDEADBEEF) {
          Serial.println("[Krypto] Nachricht erfolgreich entschlüsselt!");
          Serial.printf("App: %s | Titel: %s | Msg: %s\n",
                        receivedMsg.data.appName,
                        receivedMsg.data.title,
                        receivedMsg.data.body);

          // Signalstärken ausgeben
          SX1262_OP.receive_rssi = radio.getRSSI();
          SX1262_OP.receive_snr = radio.getSNR();
          Serial.printf("[LoRa] RSSI: %.1f dBm | SNR: %.1f dB\n", SX1262_OP.receive_rssi, SX1262_OP.receive_snr);

          // 5. LED_2 einschalten als Empfangsbestätigung
          digitalWrite(LED_2, LOW);

          // 6. E-Paper-Display beschreiben
          display.clearBuffer();

          // Wir nutzen hier einfache Standard-Schrift-Koordinaten
          display.setFont(&FreeSans9pt7b);
          display.setCursor(5, 20);
          display.setTextWrap(true);

          display.printf("%s\n %s\n %s", receivedMsg.data.appName, receivedMsg.data.title, receivedMsg.data.body);

          // Signalstärke unten klein einblenden
          display.setFont(&FreeMonoBold9pt7b);
          // display.setCursor(5, 170);
          // display.printf("RSSI %.0fdBm", SX1262_OP.receive_rssi);

          // Zeitstempel umwandeln
          String timeString = getFormattedTime(receivedMsg.packetCounter);
          // Test-Ausgabe im Seriellen Monitor
          Serial.print("Nachricht empfangen um: ");
          Serial.println(timeString);
          display.setCursor(135, 170);
          display.print(timeString);
          ;

          // Display-Refresh im nächsten Loop-Durchlauf triggern
          Display_Refresh_OP.sx1262_test.transmission_fast_refresh_flag = true;
        } else {
          Serial.println("[Krypto] Warnung: Magic Word falsch! Paket manipuliert oder falscher Key.");
        }
      } else {
        Serial.printf("[LoRa] Rx-Fehler beim Auslesen: %d\n", state);
      }

      // WICHTIG: Den Empfänger sofort wieder in den "Dauerhör-Modus" versetzen
      radio.startReceive();
    }
  }
}

bool SX1262_Initialization(void) {
  Custom_SPI_3.begin();
  Custom_SPI_3.setClockDivider(SPI_CLOCK_DIV2);

  int16_t state = -1;
  if (SX1262_OP.current_mode == SX1262_OP.mode::LORA) {
    state = radio.begin();
  } else {
    state = radio.beginFSK();
  }

  if (state == RADIOLIB_ERR_NONE) {
    String temp_str;
    if (SX1262_Set_Default_Parameters(&temp_str) == false) {
      Serial.printf("SX1262 Failed to set default parameters\n");
      Serial.printf("SX1262 assertion: %s\n", temp_str.c_str());
      return false;
    }
    if (radio.startReceive() != RADIOLIB_ERR_NONE) {
      Serial.printf("SX1262 Failed to start receive\n");
      return false;
    }
  } else {
    Serial.printf("SX1262 initialization failed\n");
    Serial.printf("Error code: %d\n", state);
    return false;
  }

  Serial.printf("SX1262 initialization successful\n");

  return true;
}

void setup(void) {
  Serial.begin(115200);

  // 3.3V Power ON
  pinMode(RT9080_EN, OUTPUT);
  digitalWrite(RT9080_EN, HIGH);

  pinMode(SCREEN_BS1, OUTPUT);
  digitalWrite(SCREEN_BS1, LOW);

  pinMode(SX1262_RF_VC1, OUTPUT);
  pinMode(SX1262_RF_VC2, OUTPUT);

  // receive
  Set_SX1262_RF_Transmitter_Switch(false);

  pinMode(nRF52840_BOOT, INPUT_PULLUP);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  digitalWrite(LED_1, HIGH);
  digitalWrite(LED_2, HIGH);

  radio.setDio1Action(Dio1_Action_Interrupt);

  display.begin();
  display.setRotation(1);
  display.setTextColor(EPD_BLACK);

  GFX_Print_SX1262_Info();
  if (SX1262_Initialization() == true) {
    GFX_Print_SX1262_Init_Successful_Refresh_Info();
    SX1262_OP.initialization_flag = true;
  } else {
    GFX_Print_SX1262_Init_Failed_Refresh_Info();
    SX1262_OP.initialization_flag = false;
  }
  display.display();

  Display_Refresh_OP.sx1262_test.transmission_fast_refresh_flag = true;
}

void loop() {
  if (digitalRead(nRF52840_BOOT) == LOW) {
    digitalWrite(LED_2, HIGH);
  }

  GFX_Print_SX1262_Info_Loop();
}
