/**
 * @file menuHandler.h
 *
 */

#ifndef MENU_HANDLER_H
#define MENU_HANDLER_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
static void back_event_handler(lv_event_t *e);
static void eventGestureDefaultCB(lv_event_t *e);
static void start_wifi_manager_timer_cb(lv_timer_t *t);
static void eventFunctionWifiCB(lv_event_t *e);
static void eventFunctionTotpCB(lv_event_t *e);
static lv_obj_t *subAboutFunction(lv_obj_t *menu);
static lv_obj_t *subLoRaMsgFunction(lv_obj_t *menu);
static lv_obj_t *subWifiFunction(lv_obj_t *menu);
static lv_obj_t *subBatteryFunction(lv_obj_t *menu);
static void update_totp_status(lv_timer_t *timer);
static lv_obj_t *subTotpFunction(lv_obj_t *menu);
void menuHandler();
static bool setupWifi();

/**********************
 *      MACROS
 **********************/

#endif /*MENU_HANDLER_H*/
