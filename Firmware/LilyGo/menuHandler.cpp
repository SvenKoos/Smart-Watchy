#include <lvgl.h>
#include <LilyGoLib.h>
#include <WiFiManager.h>
#include <esp_mac.h>
#include <esp_system.h>

#include "config.h"
#include "watchFace.h"
#include "timerEvent.h"
#include "dataCollection.h"

extern int guiState;

static void back_event_handler(lv_event_t *e) {
  lv_obj_t *obj = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  if (lv_menu_back_button_is_root(menu, obj)) {
    startBrightnessTimer(10);

    // draw the watchface screen
    guiState = WATCHFACE_STATE;
    drawWatchFace();
  }
}

// About page
static lv_obj_t *sub1Function(lv_obj_t *menu) {
  /*Create a sub page*/
  lv_obj_t *pageSub = lv_menu_page_create(menu, NULL);
  lv_obj_t *contSub = lv_menu_cont_create(pageSub);

  lv_obj_t *label = lv_label_create(contSub);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_18, LV_PART_MAIN);
  lv_obj_set_width(label, lv_pct(95)); 
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);

  char aboutText[128] = "";
  String localIP;
  String gatewayIP;
  String macAdress;

  // WiFi
  if (connectWiFi(localIP, gatewayIP, macAdress)) {
    strcpy(aboutText, "Local IP ");
    strcat(aboutText, localIP.c_str());
    strcat(aboutText, "\n");
    strcat(aboutText, "Router IP ");
    strcat(aboutText, gatewayIP.c_str());
    strcat(aboutText, "\n");
    strcat(aboutText, "WiFi MAC ");
    strcat(aboutText, macAdress.c_str());
    strcat(aboutText, "\n");
  } else {
    strcpy(aboutText, "WiFi Not Configured\n");
  }
  disconnectWifi();

  // BLE
  uint8_t mac[6];
  char bleMAC[128] = "";
  esp_read_mac(mac, ESP_MAC_BT);  // ESP_MAC_BT = BLE MAC
  snprintf(bleMAC, 128, "BLE MAC %02X:%02X:%02X:%02X:%02X:%02X\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  strcat(aboutText, bleMAC);

  lv_label_set_text(label, aboutText);

  return pageSub;
}

void eventCB(lv_event_t *e) {
  lv_obj_t *cont = (lv_obj_t *)lv_event_get_target(e);
  lv_obj_t *menu = (lv_obj_t *)lv_event_get_user_data(e);

  startBrightnessTimer(10);
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

  /*Modify the header*/
  lv_obj_t *back_btn = lv_menu_get_main_header_back_button(menu);
  lv_obj_t *back_button_label = lv_label_create(back_btn);
  lv_label_set_text(back_button_label, " Back");

  //Create a main page
  lv_obj_t *pageMain = lv_menu_page_create(menu, NULL);
  lv_obj_t *contMain = lv_menu_cont_create(pageMain);

  lv_obj_t * spinner = lv_spinner_create(lv_screen_active());
  lv_obj_set_size(spinner, 80, 80);
  lv_obj_center(spinner);
  lv_timer_handler();

  // menu item 1:
  lv_obj_t *labelSub1 = lv_label_create(contMain);
  lv_label_set_text(labelSub1, "    About");
  lv_menu_set_load_page_event(menu, contMain, sub1Function(menu));
  lv_obj_add_event_cb(contMain, eventCB, LV_EVENT_ALL, menu);

  lv_obj_delete(spinner);

  lv_menu_set_page(menu, pageMain);
}
