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

extern int guiState;
extern lilygoSettings settings;
extern uint8_t batteryHistory[1440];

static lv_obj_t *labelWifi;
static lv_obj_t *labelAbout;
static lv_obj_t *chartBattery;
static lv_obj_t *labelTOTP;
static lv_obj_t *barTOTP;
static lv_obj_t *tabviewLoRa;

lv_timer_t *timerTOTP = NULL;

const char *msgTypes[] = {
  "I'll call you later.", "Have a nice day!", "Love you!", "This is a test message."
};

static void back_event_handler(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  Serial.println("Back pressed");

  if (lv_menu_back_button_is_root(menu, obj)) {
    startBrightnessTimer(15);

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
  if ((code == LV_EVENT_SCROLL_END) || (code == LV_EVENT_GESTURE) || (code == LV_EVENT_CLICKED) || 
      (code == LV_EVENT_SCROLL) || (code == LV_EVENT_VALUE_CHANGED) || (code == LV_EVENT_STATE_CHANGED)) {

    const char * name = lv_event_code_get_name(code);
    if (name != NULL)
    { 
      Serial.print("Event code / name: "); Serial.print(code, DEC); Serial.print(" / "); Serial.println(name); 
    }
    else
    {
      Serial.print("Event code: "); Serial.println(code, DEC); 
    }

    startBrightnessTimer(15);
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

// sub page: LoR messages page
static lv_obj_t *subLoRaMsgFunction(lv_obj_t *menu) {
  Serial.println("LoRs Msg. function started");

  // spinner
  lv_refr_now(NULL);

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);
  lv_obj_set_size(contSub, lv_pct(100), lv_pct(66)); // width, height
  lv_obj_set_flex_flow(contSub, LV_FLEX_FLOW_COLUMN);  // Layout-Hilfe

  // Create a tab view with tabs on the bottom (e.g. as dot indicators)
  tabviewLoRa = lv_tabview_create(contSub);
  lv_obj_set_size(tabviewLoRa, lv_pct(100), lv_pct(100)); // Füllt den Container
  lv_tabview_set_tab_bar_position(tabviewLoRa, LV_DIR_BOTTOM);
  lv_obj_set_style_text_font(tabviewLoRa, &lv_font_montserrat_18, 0);
  registerDefaultEvents(tabviewLoRa);

  lv_obj_t * tab_btns = lv_tabview_get_tab_btns(tabviewLoRa);
  lv_obj_set_height(tab_btns, 20); // Sehr schmale Leiste
  lv_obj_set_style_bg_opa(tab_btns, LV_OPA_TRANSP, LV_PART_MAIN); // Hintergrund der Leiste unsichtbar

  int msgCount = sizeof(msgTypes) / sizeof(msgTypes[0]);  
  for (int i = 0; i < msgCount; i++) {
    // Add your tabs/pages (you can keep the names empty if you just want indicator dots)
    lv_obj_t *tab = lv_tabview_add_tab(tabviewLoRa, "*");

    lv_obj_t * label = lv_label_create(tab);
    lv_obj_set_style_text_font(label, &lv_font_montserrat_18, 0);
    lv_obj_set_width(label, lv_pct(100)); // Volle Breite des Tabs
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP); // Text umbrechen
    lv_label_set_text(label, msgTypes[i]);
  }

  // Trigger the carousel to flip to the first tab with an animation
  lv_tabview_set_active(tabviewLoRa, 0, LV_ANIM_ON);

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
      lv_chart_set_next_value(chartBattery, ser, batteryHistory[i]);
    else
      lv_chart_set_next_value(chartBattery, ser, LV_CHART_POINT_NONE);
  }

  // 1. Linien-Objekt erstellen
  lv_obj_t *time_line = lv_line_create(contSub);
  // 2. Punkte für die vertikale Linie definieren (Start oben, Ende unten)
  // Wir nutzen statische Punkte, die wir später per Code verschieben
  static lv_point_precise_t line_points[] = { { 0, 0 }, { 0, 130 } };  // 130 ist die Höhe deines Charts
  lv_line_set_points(time_line, line_points, 2);
  // 3. Styling des Strichs
  lv_obj_set_style_line_width(time_line, 2, 0);
  lv_obj_set_style_line_color(time_line, lv_palette_main(LV_PALETTE_BLUE), 0);  // Blau für Zeit
  lv_obj_set_style_line_opa(time_line, 180, 0);                                 // Leicht transparent
  lv_obj_set_style_line_dash_width(time_line, 4, 0);                            // Optional: Gestrichelt
  lv_obj_set_style_line_dash_gap(time_line, 2, 0);
  // 4. Zeit abrufen
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int totalMinutes = timeinfo.tm_hour * 60 + timeinfo.tm_min;

    // Berechne die X-Position relativ zum Chart
    int xPos = chartXStart + (totalMinutes * chartWidth / 1440);

    // Position des Strichs setzen (Y ist 20, wie dein Chart)
    lv_obj_set_pos(time_line, xPos, chartYStart);
    lv_obj_move_foreground(time_line);
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
  lv_obj_set_style_text_font(menu, &lv_font_montserrat_36, LV_PART_MAIN);

  // spinner
  lv_obj_t *spinner = lv_spinner_create(lv_screen_active());
  lv_obj_set_size(spinner, 80, 80);
  lv_obj_center(spinner);

  /*Modify the header*/
  lv_obj_t *back_btn = lv_menu_get_main_header_back_button(menu);
  lv_obj_t *back_button_label = lv_label_create(back_btn);
  lv_label_set_text(back_button_label, " Back");

  lv_obj_t *cont;
  lv_obj_t *label;

  //Create a main page
  lv_obj_t *pageMain = lv_menu_page_create(menu, NULL);

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
