#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>
#include <esp_mac.h>
#include <esp_system.h>
#include <WiFiManager.h>

#include "config.h"
#include "settings.h"
#include "watchFace.h"
#include "timerEvent.h"
#include "dataCollection.h"
#include "menuHandler.h"
#include "TOTP.h"
#include "alertData.h"
#include "lora.h"

extern int weatherIntervalCounter;
extern int locationIntervalCounter;
extern int guiState;
extern lilygoSettings settings;
extern uint8_t batteryCapacityHistory[1440];
extern uint16_t batteryVoltageHistory[1440];
extern uint16_t stepCounterHistory[1440];

extern lv_color_t color_bg;
extern lv_color_t color_text;

extern int watchType;

static lv_obj_t *pageMain;
static lv_obj_t *labelWifi;
static lv_obj_t *labelAbout;
static lv_obj_t *chartBattery;
static lv_obj_t *labelTOTP;
static lv_obj_t *barTOTP;
static lv_obj_t *rollerLoRa;
static lv_obj_t *chartStepCounter;
static lv_obj_t *labelWatchType;
static lv_obj_t *labelRefreshData;

lv_timer_t *timerTOTP = NULL;

const char *msgTypes[] = {
  "I'll call you later.", "Have a nice day!", "Love you!", "This is a test message."
};

static void back_event_handler(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  Serial.println("Back pressed");

  if (lv_menu_back_button_is_root(menu, obj)) {
    startBrightnessTimer(BRIGHTNESS_TIMEOUT_MENU);

    // draw the watchface screen
    guiState = WATCHFACE_STATE;
    drawWatchFace();
  }
}

// sub CB: Default
static void eventGestureDefaultCB(lv_event_t *e) {
  // 1. Den Event-Code abrufen
  lv_event_code_t code = lv_event_get_code(e);

  // 2. Den Code prüfen
  if ((code == LV_EVENT_SCROLL_END) || (code == LV_EVENT_GESTURE) || (code == LV_EVENT_CLICKED) || (code == LV_EVENT_SCROLL) || (code == LV_EVENT_VALUE_CHANGED) || (code == LV_EVENT_STATE_CHANGED)) {

    const char *name = lv_event_code_get_name(code);
    if (name != NULL) {
      Serial.print("Event code / name: ");
      Serial.print(code, DEC);
      Serial.print(" / ");
      Serial.println(name);
    } else {
      Serial.print("Event code: ");
      Serial.println(code, DEC);
    }

    startBrightnessTimer(BRIGHTNESS_TIMEOUT_MENU);
  }
}

static void registerDefaultEvents(lv_obj_t *cont) {
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_SCROLL_END, NULL);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_GESTURE, NULL);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_SCROLL, NULL);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_VALUE_CHANGED, NULL);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_STATE_CHANGED, NULL);
}

static void start_wifi_manager_timer_cb(lv_timer_t *t) {
  // Diese Funktion wird erst aufgerufen, wenn LVGL mit dem Seitenwechsel fertig ist
  setupWifi();
}

// sub CB: Wifi
static void eventFunctionWifiCB(lv_event_t *e) {
  // lv_obj_t *cont = (lv_obj_t *)lv_event_get_target(e);
  // lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  // 1. Den Event-Code abrufen
  lv_event_code_t code = lv_event_get_code(e);

  // 2. Den Code prüfen
  if (code == LV_EVENT_CLICKED) {
    Serial.println("WiFi Pressed");

    startBrightnessTimer(WIFI_AP_TIMEOUT + 10);

    lv_timer_t *timer = lv_timer_create(start_wifi_manager_timer_cb, 100, NULL);
    lv_timer_set_repeat_count(timer, 1);
  }
}

// sub CB: TOTP
static void eventFunctionTotpCB(lv_event_t *e) {
  // lv_obj_t *cont = (lv_obj_t *)lv_event_get_target(e);
  // lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  // 1. Den Event-Code abrufen
  lv_event_code_t code = lv_event_get_code(e);

  // 2. Den Code prüfen
  if (code == LV_EVENT_CLICKED) {
    Serial.println("TOTP Pressed");

    startBrightnessTimer(30);

    lv_label_set_text(labelTOTP, calculateTotpCode().c_str());
  }
}

// sub page: About page
static lv_obj_t *subAboutFunction(lv_obj_t *menu) {
  Serial.println("About function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  labelAbout = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelAbout, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_width(labelAbout, lv_pct(95));
  lv_label_set_long_mode(labelAbout, LV_LABEL_LONG_WRAP);
  registerDefaultEvents(labelAbout);

  char aboutText[128] = "";
  String localIP;
  String gatewayIP;
  String macAdress;

  // connect WiFi
  if (connectWiFi(localIP, gatewayIP, macAdress)) {
    strcpy(aboutText, "About Smart Watchy:\n - Local IP\n   ");
    strcat(aboutText, localIP.c_str());
    strcat(aboutText, "\n - Router IP\n   ");
    strcat(aboutText, gatewayIP.c_str());
    strcat(aboutText, "\n - WiFi MAC\n   ");
    strcat(aboutText, macAdress.c_str());
  } else {
    strcpy(aboutText, "WiFi Not Configured\n");
  }
  // spinner
  lv_refr_now(NULL);

  // disconnect WiFi
  disconnectWifi();

  // BLE
  uint8_t mac[6];
  char bleMAC[128] = "";
  esp_read_mac(mac, ESP_MAC_BT);  // ESP_MAC_BT = BLE MAC
  snprintf(bleMAC, 128, "\n - BLE MAC\n   %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  strcat(aboutText, bleMAC);

  lv_label_set_text(labelAbout, aboutText);

  return pageSub;
}

static void eventRollerLoRaCB(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t *roller = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  // Wenn der Benutzer den Roller dreht und stoppt (Auswahl ändert sich)
  if (code == LV_EVENT_VALUE_CHANGED) {
    uint16_t sel_idx = lv_roller_get_selected(roller);
    char buf[32];
    lv_roller_get_selected_str(roller, buf, sizeof(buf));
    Serial.printf("Roller gedreht. Index: %d, Text: %s\n", sel_idx, buf);

    startBrightnessTimer(BRIGHTNESS_TIMEOUT_MENU);
  }

  // Wenn der Benutzer fest auf die aktuell ausgewählte Nachricht TIPPTE (Senden)
  if (code == LV_EVENT_CLICKED) {
    uint16_t sel_idx = lv_roller_get_selected(roller);
    Serial.printf("Nachricht ausgewählt zum Senden! Index: %d (Text: %s)\n", sel_idx, msgTypes[sel_idx]);

    addMsgToLora(msgTypes[sel_idx]);

    lv_obj_send_event(lv_menu_get_main_header_back_button(menu), LV_EVENT_CLICKED, NULL);
  }
}

// sub page: LoR messages page
static lv_obj_t *subLoRaMsgFunction(lv_obj_t *menu) {
  Serial.println("LoRs Msg. function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);
  lv_obj_set_size(contSub, lv_pct(100), lv_pct(100));  // width, height
  lv_obj_set_layout(contSub, LV_LAYOUT_NONE);

  /* 2. Den Roller (die Text-Walze) erstellen */
  rollerLoRa = lv_roller_create(contSub);
  lv_obj_set_size(rollerLoRa, lv_pct(100), lv_pct(75));  // Füllt den Container
  // Optionen setzen (Modus: INFINITE erlaubt endloses Durchscrollen im Kreis, normal wäre NORMAL)
  lv_obj_set_style_text_font(rollerLoRa, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_font(rollerLoRa, &lv_font_montserrat_18, LV_PART_SELECTED);  // Ausgewählter Text
  // Sichtbare Zeilenanzahl automatisch an die Höhe anpassen (z.B. 3 Zeilen sichtbar)
  lv_roller_set_visible_row_count(rollerLoRa, 3);
  // Mittig im oberen/mittleren Bereich platzieren
  lv_obj_align(rollerLoRa, LV_ALIGN_TOP_MID, 0, 0);
  // WICHTIG: Den Start-Index unmissverständlich auf 0 (erste Nachricht) zwingen
  lv_roller_set_selected(rollerLoRa, 0, LV_ANIM_OFF);
  // Unseren neuen, schlanken Roller-Callback zuweisen
  lv_obj_add_event_cb(rollerLoRa, eventRollerLoRaCB, LV_EVENT_ALL, menu);

  int msgCount = sizeof(msgTypes) / sizeof(msgTypes[0]);
  // Alle Nachrichten aus deinem msgTypes-Array für den Roller zu einem String verbinden
  // Die Optionen müssen in LVGL durch ein '\n' (New Line) getrennt sein
  String rollerOptions = "";
  for (int i = 0; i < msgCount; i++) {
    rollerOptions += msgTypes[i];
    if (i < msgCount - 1) rollerOptions += "\n";
  }
  lv_roller_set_options(rollerLoRa, rollerOptions.c_str(), LV_ROLLER_MODE_INFINITE);

  lv_obj_t *labelHint = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelHint, &lv_font_montserrat_18, 0);
  lv_obj_set_style_text_align(labelHint, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_width(labelHint, lv_pct(95));                // Volle Breite des Tabs
  lv_label_set_long_mode(labelHint, LV_LABEL_LONG_WRAP);  // Text umbrechen
  lv_label_set_text(labelHint, "Tap on selected message to send.");
  lv_obj_align(labelHint, LV_ALIGN_BOTTOM_MID, 0, -5);

  return pageSub;
}

// sub page: Configure WiFi page
static lv_obj_t *subWifiFunction(lv_obj_t *menu) {
  Serial.println("Wifi function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  labelWifi = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelWifi, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_width(labelWifi, lv_pct(95));
  lv_label_set_long_mode(labelWifi, LV_LABEL_LONG_WRAP);
  registerDefaultEvents(labelWifi);

  char wifiText[256] = "Configure WiFi:\n - Connect to SSID\n   ";
  strcat(wifiText, WIFI_AP_SSID);
  strcat(wifiText, "\n - Connect to Portal IP\n   192.168.4.1");
  strcat(wifiText, "\n - Waiting for 60sec.\n - Don't press the Back button!");
  lv_label_set_text(labelWifi, wifiText);

  return pageSub;
}

// sub page: battery history page
static lv_obj_t *subBatteryFunction(lv_obj_t *menu) {
  Serial.println("Battery function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);
  // Deaktiviert das Flex-Layout, damit align_to funktioniert
  lv_obj_set_layout(contSub, LV_LAYOUT_NONE);

  int chartWidth = 160;
  int chartXStart = 40;
  int chartYStart = 5;

  chartBattery = lv_chart_create(contSub);
  lv_chart_set_type(chartBattery, LV_CHART_TYPE_LINE);
  registerDefaultEvents(chartBattery);
  lv_obj_set_size(chartBattery, chartWidth, 130);          // Etwas kleiner als das Display
  lv_obj_set_pos(chartBattery, chartXStart, chartYStart);  // X=55 lässt genug Platz für die Y-Labels links
  lv_chart_set_axis_range(chartBattery, LV_CHART_AXIS_PRIMARY_Y, 0, 100);
  lv_chart_set_point_count(chartBattery, 1440);

  // Skala für die Y-Achse erstellen
  lv_obj_t *scale_y = lv_scale_create(contSub);
  lv_obj_set_size(scale_y, 45, 130);  // Gleiche Höhe wie der Chart
  lv_obj_align_to(scale_y, chartBattery, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  // Skala konfigurieren
  lv_scale_set_mode(scale_y, LV_SCALE_MODE_VERTICAL_LEFT);
  lv_scale_set_range(scale_y, 0, 100);
  lv_scale_set_total_tick_count(scale_y, 3);  // 0, 50, 100
  lv_scale_set_major_tick_every(scale_y, 1);  // Jeder Tick bekommt ein Label
  lv_obj_set_style_text_font(scale_y, &lv_font_montserrat_18, LV_PART_MAIN);
  // Labels setzen
  static const char *y_labels[] = { "0/\n3.5\n", "50/\n4.0", "\n100/\n4.5", NULL };
  lv_scale_set_text_src(scale_y, y_labels);

  // Skala für die X-Achse erstellen
  lv_obj_t *scale_x = lv_scale_create(contSub);
  lv_obj_set_size(scale_x, 160, 40);  // Gleiche Breite wie der Chart
  lv_obj_align_to(scale_x, chartBattery, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
  lv_scale_set_mode(scale_x, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
  lv_scale_set_range(scale_x, 0, 24);
  lv_scale_set_total_tick_count(scale_x, 3);  // 0, 12, 24
  lv_scale_set_major_tick_every(scale_x, 1);
  lv_obj_set_style_text_font(scale_x, &lv_font_montserrat_18, LV_PART_MAIN);
  static const char *x_labels[] = { "0", "12", "24", NULL };
  lv_scale_set_text_src(scale_x, x_labels);

  // spinner
  lv_refr_now(NULL);

  // battery capacity
  lv_chart_series_t *serCapacity = lv_chart_add_series(chartBattery, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  for (int i = 0; i < 1440; i++) {
    if (batteryCapacityHistory[i] > 0)
      lv_chart_set_value_by_id(chartBattery, serCapacity, i, batteryCapacityHistory[i]);
    else
      lv_chart_set_value_by_id(chartBattery, serCapacity, i, LV_CHART_POINT_NONE);
  }

  // battery voltage
  lv_chart_series_t *serVoltage = lv_chart_add_series(chartBattery, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  for (int i = 0; i < 1440; i++) {
    uint16_t rawVoltage = batteryVoltageHistory[i];

    if ((rawVoltage >= 3500) && (rawVoltage <= 4500)) {
      // Mathematisches Mapping in das 0-100er Raster
      int32_t scaledVoltageValue = (rawVoltage - 3500) / 10;
      lv_chart_set_value_by_id(chartBattery, serVoltage, i, scaledVoltageValue);
    } else {
      lv_chart_set_value_by_id(chartBattery, serVoltage, i, LV_CHART_POINT_NONE);
    }
  }

  return pageSub;
}

// sub page: step counter history page
static lv_obj_t *subStepCounterFunction(lv_obj_t *menu) {
  Serial.println("Step counter function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);
  // Deaktiviert das Flex-Layout, damit align_to funktioniert
  lv_obj_set_layout(contSub, LV_LAYOUT_NONE);

  int chartWidth = 160;
  int chartXStart = 40;
  int chartYStart = 5;

  chartStepCounter = lv_chart_create(contSub);
  lv_chart_set_type(chartStepCounter, LV_CHART_TYPE_LINE);
  registerDefaultEvents(chartStepCounter);
  lv_obj_set_size(chartStepCounter, chartWidth, 130);          // Etwas kleiner als das Display
  lv_obj_set_pos(chartStepCounter, chartXStart, chartYStart);  // X=55 lässt genug Platz für die Y-Labels links
  lv_chart_set_axis_range(chartStepCounter, LV_CHART_AXIS_PRIMARY_Y, 0, 20000);
  lv_chart_set_range(chartStepCounter, LV_CHART_AXIS_PRIMARY_Y, 0, 20000);
  lv_chart_set_point_count(chartStepCounter, 1440);

  // Skala für die Y-Achse erstellen
  lv_obj_t *scale_y = lv_scale_create(contSub);
  lv_obj_set_size(scale_y, 45, 130);  // Gleiche Höhe wie der Chart
  lv_obj_align_to(scale_y, chartStepCounter, LV_ALIGN_OUT_LEFT_MID, -5, 0);
  // Skala konfigurieren
  lv_scale_set_mode(scale_y, LV_SCALE_MODE_VERTICAL_LEFT);
  lv_scale_set_range(scale_y, 0, 20000);
  lv_scale_set_total_tick_count(scale_y, 3);  // 0, 10k, 20k
  lv_scale_set_major_tick_every(scale_y, 1);  // Jeder Tick bekommt ein Label
  lv_obj_set_style_text_font(scale_y, &lv_font_montserrat_18, LV_PART_MAIN);
  // Labels setzen
  static const char *y_labels[] = { "0", "10k", "20k", NULL };
  lv_scale_set_text_src(scale_y, y_labels);

  // Skala für die X-Achse erstellen
  lv_obj_t *scale_x = lv_scale_create(contSub);
  lv_obj_set_size(scale_x, 160, 40);  // Gleiche Breite wie der Chart
  lv_obj_align_to(scale_x, chartStepCounter, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
  lv_scale_set_mode(scale_x, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
  lv_scale_set_range(scale_x, 0, 24);
  lv_scale_set_total_tick_count(scale_x, 3);  // 0, 12, 24
  lv_scale_set_major_tick_every(scale_x, 1);
  lv_obj_set_style_text_font(scale_x, &lv_font_montserrat_18, LV_PART_MAIN);
  static const char *x_labels[] = { "0", "12", "24", NULL };
  lv_scale_set_text_src(scale_x, x_labels);

  // spinner
  lv_refr_now(NULL);

  lv_chart_series_t *ser = lv_chart_add_series(chartStepCounter, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  for (int i = 0; i < 1440; i++) {
    if (stepCounterHistory[i] > 0)
      // lv_chart_set_next_value(chartStepCounter, ser, stepCounterHistory[i]);
      lv_chart_set_value_by_id(chartStepCounter, ser, i, stepCounterHistory[i]);
    else
      lv_chart_set_value_by_id(chartStepCounter, ser, i, LV_CHART_POINT_NONE);
  }

  return pageSub;
}

// Event-Callback für die Radio-Buttons
static void watch_type_event_cb(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  uint32_t id = lv_buttonmatrix_get_selected_button(obj);

  if (id == 0) {
    watchType = ANALOGUE_WATCH;
    Serial.println("Watch type set to: ANALOGUE");
  } else if (id == 1) {
    watchType = DIGITAL_WATCH;
    Serial.println("Watch type set to: DIGITAL");
  } else if (id == 2) {
    watchType = QLOCKTWO_WATCH;
    Serial.println("Watch type set to: QLOCKTWO");
  }
  // Hier kannst du in Zukunft einfach erweitern: else if(id == 2) { ... }
}

static lv_obj_t *subWatchTypeFunction(lv_obj_t *menu) {
  Serial.println("Watch type function started");

  // Spinner/Refresh erzwingen
  lv_refr_now(NULL);

  /* Sub-Page und Container erstellen */
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);

  // Wir nutzen hier ein einfaches Flex-Layout im Container, damit die Elemente untereinander stehen
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);
  lv_obj_set_flex_flow(contSub, LV_FLEX_FLOW_COLUMN);

  // 1. Statischer Titel oben
  labelWatchType = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelWatchType, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_color(labelWatchType, color_text, LV_PART_MAIN);
  lv_label_set_text(labelWatchType, "Watch Type");
  registerDefaultEvents(labelWatchType);  // Deine Event-Registrierung beibehalten

  // 2. Die Radio-Button-Matrix erstellen
  // WICHTIG: Die Namen der Buttons. Das "\n" sorgt dafür, dass sie UNTEREINANDER stehen!
  static const char *btn_map[] = { "Analog", "\n", "Digital", "\n", "QlockTwo", "" };

  lv_obj_t *radio_matrix = lv_buttonmatrix_create(contSub);
  lv_buttonmatrix_set_map(radio_matrix, btn_map);
  lv_obj_set_width(radio_matrix, lv_pct(100));
  lv_obj_set_height(radio_matrix, 180); // Setze eine größere Höhe

  // Button-Matrix optisch aufräumen (kein Hintergrund, flacher Look)
  lv_obj_set_style_bg_opa(radio_matrix, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(radio_matrix, 0, 0);

  // One-Check aktivieren (wirkt wie Radio-Buttons: nur einer kann aktiv sein)
  lv_buttonmatrix_set_one_checked(radio_matrix, true);

  // Buttons als "Checkable" (einrastend) markieren
  lv_buttonmatrix_set_button_ctrl_all(radio_matrix, LV_BUTTONMATRIX_CTRL_CHECKABLE);

  // 3. Den aktuell aktiven Status beim Öffnen der Seite vorselektieren
  if (watchType == ANALOGUE_WATCH) {
    lv_buttonmatrix_set_button_ctrl(radio_matrix, 0, LV_BUTTONMATRIX_CTRL_CHECKED);
  } else if (watchType == DIGITAL_WATCH) {
    lv_buttonmatrix_set_button_ctrl(radio_matrix, 1, LV_BUTTONMATRIX_CTRL_CHECKED);
  } else {
    lv_buttonmatrix_set_button_ctrl(radio_matrix, 2, LV_BUTTONMATRIX_CTRL_CHECKED);
  }

  lv_obj_set_style_radius(radio_matrix, 8, LV_PART_ITEMS);  // Leicht abgerundete Ecken für die Buttons
  lv_obj_set_style_text_font(radio_matrix, &lv_font_montserrat_18, LV_PART_MAIN);

  // Event-Handler anhängen
  lv_obj_add_event_cb(radio_matrix, watch_type_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

  return pageSub;
}

// sub page: TOTP page
// Diese Funktion wird vom Timer aufgerufen (z.B. jede Sekunde)
static void update_totp_status(lv_timer_t *timer) {
  time_t now;
  time(&now);
  struct tm timeinfo;
  localtime_r(&now, &timeinfo);

  // 1. Berechne verbleibende Sekunden im 30s Fenster
  int remaining = 30 - (timeinfo.tm_sec % 30);

  // 2. Bar aktualisieren (Sollte von 30 auf 0 sinken)
  lv_bar_set_value(barTOTP, remaining, LV_ANIM_ON);

  if (remaining == 30 || remaining == 29) {
    lv_label_set_text(labelTOTP, calculateTotpCode().c_str());
  }
}

static lv_obj_t *subTotpFunction(lv_obj_t *menu) {
  Serial.println("TOTP function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  lv_obj_set_layout(contSub, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(contSub, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_flex_align(contSub, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // 1. Account Name
  lv_obj_t *label_name = lv_label_create(contSub);
  lv_label_set_text(label_name, settings.totpAccount.c_str());
  lv_obj_set_style_text_font(label_name, &lv_font_montserrat_18, 0);

  // 2. Der Code (Groß)
  labelTOTP = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelTOTP, &lv_font_montserrat_48, 0);  // Schön groß
  lv_obj_set_style_text_letter_space(labelTOTP, 5, 0);

  // 3. Fortschrittsbalken (Die verbleibenden Sekunden der 30s)
  barTOTP = lv_bar_create(contSub);
  lv_obj_set_size(barTOTP, 180, 12);
  lv_bar_set_range(barTOTP, 0, 30);  // 0 bis 30 Sekunden
  lv_bar_set_value(barTOTP, 30, LV_ANIM_OFF);
  lv_timer_t *timerTOTP = lv_timer_create(update_totp_status, 1000, NULL);
  lv_obj_add_event_cb(
    contSub, [](lv_event_t *e) {
      lv_timer_t *t = (lv_timer_t *)lv_event_get_user_data(e);
      lv_timer_delete(t);
    },
    LV_EVENT_DELETE, timerTOTP);

  return pageSub;
}

// sub page: Refresh data page
static lv_obj_t *subRefreshDataFunction(lv_obj_t *menu) {
  Serial.println("Fresh data function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  labelRefreshData = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelRefreshData, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_width(labelRefreshData, lv_pct(95));
  lv_label_set_long_mode(labelRefreshData, LV_LABEL_LONG_WRAP);
  registerDefaultEvents(labelRefreshData);

  weatherIntervalCounter = -1;
  locationIntervalCounter = -1;

  char refreshDataText[256] = "";
  strcpy(refreshDataText, "Refresh of WiFi, location, weather and message data requested. Please expect up to 1min delay.\n");
  lv_label_set_text(labelRefreshData, refreshDataText);

  return pageSub;
}

void menuHandler() {
  // 1. Basis-Farben und Themes holen
  lv_color_t item_bg_color = lv_palette_darken(LV_PALETTE_BLUE_GREY, 4);  // Elegantes Anthrazit analog zur Message-Card
  lv_color_t text_color_main = lv_color_white();                          // Text knackig weiß für Lesbarkeit
  lv_color_t accent_color = GetTheme(THEME_MENU);                         // Deine System-Akzentfarbe

  // menu erstellen
  lv_obj_t *menu = lv_menu_create(lv_screen_active());
  lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
  lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL), lv_display_get_vertical_resolution(NULL));
  lv_obj_center(menu);
  lv_obj_add_event_cb(menu, eventGestureDefaultCB, LV_EVENT_SCROLL, NULL);

  // Basis-Style für das Menü-Hintergrund-System
  lv_obj_set_style_bg_color(menu, color_bg, 0);
  lv_obj_set_style_border_width(menu, 0, 0);
  lv_obj_set_style_pad_all(menu, 0, 0);  // Platz maximal ausnutzen

  //
  lv_obj_set_style_text_color(menu, lv_palette_lighten(LV_PALETTE_GREY, 2), 0);

  // Spinner für Ladezeit
  lv_obj_t *spinner = lv_spinner_create(lv_screen_active());
  lv_obj_set_size(spinner, 80, 80);
  lv_obj_center(spinner);

  // Modify the header / Back Button
  lv_obj_t *back_btn = lv_menu_get_main_header_back_button(menu);
  lv_obj_set_style_pad_top(back_btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_bottom(back_btn, 12, LV_PART_MAIN);
  lv_obj_set_style_pad_left(back_btn, 8, LV_PART_MAIN);
  // Vererbungsschutz und Hintergrundfarbe
  lv_obj_set_style_text_opa(back_btn, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_set_style_bg_color(back_btn, color_bg, LV_PART_MAIN);
  // Dein Label erstellen
  lv_obj_t *back_button_label = lv_label_create(back_btn);
  lv_obj_set_style_text_font(back_button_label, &lv_font_montserrat_26, LV_PART_MAIN);
  lv_obj_set_style_text_color(back_button_label, accent_color, LV_PART_MAIN);
  lv_obj_set_style_text_opa(back_button_label, LV_OPA_COVER, LV_PART_MAIN);
  lv_label_set_text(back_button_label, LV_SYMBOL_LEFT " Back");

  // Gemeinsamer Style für alle Menü-Container (Kacheln)
  static lv_style_t styleCont;
  lv_style_init(&styleCont);
  lv_style_set_bg_color(&styleCont, item_bg_color);
  lv_style_set_bg_opa(&styleCont, LV_OPA_COVER);
  lv_style_set_radius(&styleCont, 10);  // Schön abgerundete Ecken
  lv_style_set_border_width(&styleCont, 0);
  lv_style_set_pad_all(&styleCont, 12);       // Genug Touch-Fläche im Inneren
  lv_style_set_margin_bottom(&styleCont, 8);  // Abstand zur nächsten Kachel
  lv_style_set_margin_left(&styleCont, 8);    // Abstand zum Displayrand links
  lv_style_set_margin_right(&styleCont, 8);   // Abstand zum Displayrand rechts

  // Gemeinsamer Style für die Labeltexte
  static lv_style_t styleLabel;
  lv_style_init(&styleLabel);
  lv_style_set_text_font(&styleLabel, &lv_font_montserrat_26);  // Etwas schlanker als 34 für Icon-Platz
  lv_style_set_text_color(&styleLabel, text_color_main);

  // Create a main page
  pageMain = lv_menu_page_create(menu, NULL);

  // Färbt den Hintergrund der Hauptseite direkt ein
  lv_obj_set_style_bg_color(pageMain, color_bg, 0);

  // symbols
  // LV_SYMBOL_SETTINGS (Zahnrad) ==> Watch Type
  // LV_SYMBOL_LIST (Listen-Striche) ==> About
  // LV_SYMBOL_BULLET (Aufzählungspunkt) ==> -
  // LV_SYMBOL_OK (Häkchen) ==> -
  // LV_SYMBOL_CLOSE (Kreuz) ==> -
  // LV_SYMBOL_REFRESH (Synchronisieren) ==> Refresh Data
  // LV_SYMBOL_EDIT (Stift) ==> -
  // LV_SYMBOL_DOWNLOAD (Download) ==> -
  // LV_SYMBOL_EYE_OPEN (Auge) ==> TOTP
  // LV_SYMBOL_EYE_CLOSE (Auge) ==> -
  // LV_SYMBOL_ENVELOPE (Briefumschlag), ==> LoRa
  // LV_SYMBOL_CHARGE (Blitz) ==> Battery
  //  LV_SYMBOL_IMAGE (Bild) ==> Steps
  // LV_SYMBOL_WIFI (WLAN) ==> WiFi

  lv_obj_t *cont;
  lv_obj_t *label;

  // --- ITEM: TOTP ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventFunctionTotpCB, LV_EVENT_CLICKED, menu);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_EYE_OPEN "  TOTP");  // Schlüssel-Icon vorangestellt
  lv_menu_set_load_page_event(menu, cont, subTotpFunction(menu));

  // --- ITEM: LORA MESSAGES ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_ENVELOPE "  LoRa Msg");  // Brief-Icon
  lv_menu_set_load_page_event(menu, cont, subLoRaMsgFunction(menu));

  // --- ITEM: Refresh Data ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_REFRESH "  New Data");  // Refresh Icon
  lv_menu_set_load_page_event(menu, cont, subRefreshDataFunction(menu));

  // --- ITEM: BATTERY ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_CHARGE "  Battery");  // Batterie/Lade-Icon
  lv_menu_set_load_page_event(menu, cont, subBatteryFunction(menu));

  // --- ITEM: STEPS ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_IMAGE "  Steps");  // Platzhalter für Aktivität
  lv_menu_set_load_page_event(menu, cont, subStepCounterFunction(menu));

  // --- ITEM: Watch Type ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_SETTINGS "  Watch");  // Platzhalter für Aktivität
  lv_menu_set_load_page_event(menu, cont, subWatchTypeFunction(menu));

  // --- ITEM: ABOUT ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_LIST "  About");  // Info-i Icon
  lv_menu_set_load_page_event(menu, cont, subAboutFunction(menu));

  // --- SYSTEM SEPARATOR (Dezente Linie statt Unterstrichen) ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_set_size(cont, lv_pct(100), 20);
  lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);  // Container unsichtbar machen
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_t *line = lv_obj_create(cont);  // Echte feine Linie einziehen
  lv_obj_set_size(line, lv_pct(90), 1);
  lv_obj_center(line);
  lv_obj_set_style_bg_color(line, lv_palette_darken(LV_PALETTE_BLUE_GREY, 3), 0);
  lv_obj_set_style_border_width(line, 0, 0);

  // --- ITEM: WIFI ---
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_style(cont, &styleCont, 0);
  lv_obj_add_event_cb(cont, eventFunctionWifiCB, LV_EVENT_CLICKED, menu);
  label = lv_label_create(cont);
  lv_obj_add_style(label, &styleLabel, 0);
  lv_label_set_text(label, LV_SYMBOL_WIFI "  WiFi");  // WiFi-Icon
  lv_menu_set_load_page_event(menu, cont, subWifiFunction(menu));

  // spinner löschen
  lv_obj_delete(spinner);

  lv_menu_set_page(menu, pageMain);
}

static bool setupWifi() {
  Serial.println("Setup WiFi started");

  // Hardware-Reset für sauberen AP-Start
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(100);

  WiFiManager wifiManager;
  wifiManager.setTimeout(WIFI_AP_TIMEOUT);
  wifiManager.setConfigPortalBlocking(false);

  // Portal starten (kehrt bei non-blocking sofort zurück)
  wifiManager.startConfigPortal(WIFI_AP_SSID);
  delay(500);

  // Eigene Schleife, solange das Portal aktiv ist
  // wifiManager.process() muss regelmäßig aufgerufen werden!
  while (wifiManager.getConfigPortalActive()) {
    wifiManager.process();  // Verarbeitet WiFi-Anfragen (Webseite)
    lv_timer_handler();     // Hält LVGL/Display am Leben

    // Optional: Ein kleines delay entlastet die CPU
    yield();
    delay(10);

    // Hier könntest du prüfen, ob der User am Display "Abbrechen" drückt
  }

  char wifiText[256] = "";
  // Nachdem das Portal geschlossen wurde (durch Timeout oder Erfolg):
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected");

    settings.wifiApSSID = String(WiFi.SSID());

    strcpy(wifiText, "Connected to SSID: ");
    strcat(wifiText, settings.wifiApSSID.c_str());
    lv_label_set_text(labelWifi, wifiText);

    return true;
  } else {
    Serial.println("WiFi connection failed");

    strcpy(wifiText, "Failed to configure WiFi AP: ");
    strcat(wifiText, WIFI_AP_SSID);
    lv_label_set_text(labelWifi, wifiText);

    return false;
  }
}
