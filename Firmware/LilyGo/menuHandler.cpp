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

extern int guiState;
extern lilygoSettings settings;
extern uint8_t batteryHistory[1440];
extern uint16_t stepCounterHistory[1440];

static lv_obj_t *pageMain;
static lv_obj_t *labelWifi;
static lv_obj_t *labelAbout;
static lv_obj_t *chartBattery;
static lv_obj_t *labelTOTP;
static lv_obj_t *barTOTP;
static lv_obj_t * rollerLoRa;
static lv_obj_t *chartStepCounter;

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
  if ((code == LV_EVENT_SCROLL_END) || (code == LV_EVENT_GESTURE) || (code == LV_EVENT_CLICKED) || (code == LV_EVENT_SCROLL) || 
      (code == LV_EVENT_VALUE_CHANGED) || (code == LV_EVENT_STATE_CHANGED)) {

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
  lv_obj_t * roller = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t * menu = (lv_obj_t *)lv_event_get_user_data(e);

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
  lv_obj_set_size(contSub, lv_pct(100), lv_pct(100));   // width, height
  lv_obj_set_layout(contSub, LV_LAYOUT_NONE);

  /* 2. Den Roller (die Text-Walze) erstellen */
  rollerLoRa = lv_roller_create(contSub);
  lv_obj_set_size(rollerLoRa, lv_pct(100), lv_pct(75));  // Füllt den Container
  // Optionen setzen (Modus: INFINITE erlaubt endloses Durchscrollen im Kreis, normal wäre NORMAL)
  lv_obj_set_style_text_font(rollerLoRa, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_style_text_font(rollerLoRa, &lv_font_montserrat_18, LV_PART_SELECTED); // Ausgewählter Text
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
  lv_obj_set_width(labelHint, lv_pct(95));               // Volle Breite des Tabs
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
  static const char *y_labels[] = { "0", "50", "100", NULL };
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

  lv_chart_series_t *ser = lv_chart_add_series(chartBattery, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  for (int i = 0; i < 1440; i++) {
    if (batteryHistory[i] > 0)
      // lv_chart_set_next_value(chartBattery, ser, batteryHistory[i]);
      lv_chart_set_value_by_id(chartBattery, ser, i, batteryHistory[i]);
    else
      // lv_chart_set_next_value(chartBattery, ser, LV_CHART_POINT_NONE);
      lv_chart_set_value_by_id(chartBattery, ser, i, LV_CHART_POINT_NONE);
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

void menuHandler() {
  // menu
  lv_obj_t *menu = lv_menu_create(lv_screen_active());
  lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
  lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL) - 10, lv_display_get_vertical_resolution(NULL) - 10);
  lv_obj_center(menu);
  lv_obj_add_event_cb(menu, eventGestureDefaultCB, LV_EVENT_SCROLL, NULL);

  // style
  lv_obj_set_style_text_font(menu, &lv_font_montserrat_34, LV_PART_MAIN);

  // spinner
  lv_obj_t *spinner = lv_spinner_create(lv_screen_active());
  lv_obj_set_size(spinner, 80, 80);
  lv_obj_center(spinner);

  /*Modify the header*/
  lv_obj_t *back_btn = lv_menu_get_main_header_back_button(menu);
  lv_obj_t *back_button_label = lv_label_create(back_btn);
  lv_label_set_text(back_button_label, " Back");
  lv_obj_set_style_text_font(back_button_label, &lv_font_montserrat_36, LV_PART_MAIN);

  lv_obj_t *cont;
  lv_obj_t *label;

  //Create a main page
  pageMain = lv_menu_page_create(menu, NULL);

  // menu item TOTP
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventFunctionTotpCB, LV_EVENT_CLICKED, menu);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    TOTP");
  lv_menu_set_load_page_event(menu, cont, subTotpFunction(menu));

  // menu item About
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    About");
  lv_menu_set_load_page_event(menu, cont, subAboutFunction(menu));

  // menu item battery history
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    Battery");
  lv_menu_set_load_page_event(menu, cont, subBatteryFunction(menu));

  // menu item step counterhistory
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    Steps");
  lv_menu_set_load_page_event(menu, cont, subStepCounterFunction(menu));

  // menu item Lora Messages
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventGestureDefaultCB, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    LoRa Msg");
  lv_menu_set_load_page_event(menu, cont, subLoRaMsgFunction(menu));

  // separator
  cont = lv_menu_cont_create(pageMain);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    _______");

  // menu item Configure WiFi
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventFunctionWifiCB, LV_EVENT_CLICKED, menu);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    WiFi");
  lv_menu_set_load_page_event(menu, cont, subWifiFunction(menu));

  // spinner
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
