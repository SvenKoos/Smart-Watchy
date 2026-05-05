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

extern int guiState;
extern lilygoSettings settings;

static lv_obj_t *labelWifi;
static lv_obj_t *labelAbout;

static void back_event_handler(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  Serial.println("Back pressed");

  if (lv_menu_back_button_is_root(menu, obj)) {
    startBrightnessTimer(10);

    // draw the watchface screen
    guiState = WATCHFACE_STATE;
    drawWatchFace();
  }
}

// sub 1: About
static void eventFunction1CB(lv_event_t *e) {
  lv_obj_t *cont = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  // 1. Den Event-Code abrufen
  lv_event_code_t code = lv_event_get_code(e);

  // 2. Den Code prüfen
  if (code == LV_EVENT_CLICKED) {
    Serial.println("About Pressed");

    startBrightnessTimer(10);
  }
}

static void start_wifi_manager_timer_cb(lv_timer_t *t) {
  // Diese Funktion wird erst aufgerufen, wenn LVGL mit dem Seitenwechsel fertig ist
  setupWifi();
}

// sub 2: Wifi
static void eventFunction2CB(lv_event_t *e) {
  lv_obj_t *cont = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

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

// sub 2: Configure WiFi page
static lv_obj_t *sub2Function(lv_obj_t *menu) {
  Serial.println("Sub2 function started");

  // spinner
  lv_timer_handler();

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  labelWifi = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelWifi, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_width(labelWifi, lv_pct(95));
  lv_label_set_long_mode(labelWifi, LV_LABEL_LONG_WRAP);

  char wifiText[128] = "Configure WiFi:\n - Connect to SSID\n   ";
  strcat(wifiText, WIFI_AP_SSID);
  strcat(wifiText, "\n - Connect to Portal IP\n   192.168.4.1");
  strcat(wifiText, "\n - Waiting for 60sec.\n - Don't press the Back button!");
  lv_label_set_text(labelWifi, wifiText);

  return pageSub;
}

// sub 1: About page
static lv_obj_t *sub1Function(lv_obj_t *menu) {
  Serial.println("Sub1 function started");

  // spinner
  lv_timer_handler();

  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  labelAbout = lv_label_create(contSub);
  lv_obj_set_style_text_font(labelAbout, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_width(labelAbout, lv_pct(95));
  lv_label_set_long_mode(labelAbout, LV_LABEL_LONG_WRAP);

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
  lv_timer_handler();

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

void menuHandler() {
  // menu
  lv_obj_t *menu = lv_menu_create(lv_screen_active());
  lv_menu_set_mode_root_back_button(menu, LV_MENU_ROOT_BACK_BUTTON_ENABLED);
  lv_obj_add_event_cb(menu, back_event_handler, LV_EVENT_CLICKED, menu);
  lv_obj_set_size(menu, lv_display_get_horizontal_resolution(NULL) - 10, lv_display_get_vertical_resolution(NULL) - 10);
  lv_obj_center(menu);

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

  // menu item 1: About
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventFunction1CB, LV_EVENT_ALL, menu);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    About");
  lv_menu_set_load_page_event(menu, cont, sub1Function(menu));

  // menu item 2: Configure WiFi
  cont = lv_menu_cont_create(pageMain);
  lv_obj_add_event_cb(cont, eventFunction2CB, LV_EVENT_ALL, menu);
  label = lv_label_create(cont);
  lv_label_set_text(label, "    WiFi");
  lv_menu_set_load_page_event(menu, cont, sub2Function(menu));

  // spinner
  lv_obj_delete(spinner);

  lv_menu_set_page(menu, pageMain);
}

bool setupWifi() {
  Serial.println("Setup WiFi started");

  // Hardware-Reset für sauberen AP-Start
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(100);

  WiFiManager wifiManager;
  wifiManager.resetSettings();
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
