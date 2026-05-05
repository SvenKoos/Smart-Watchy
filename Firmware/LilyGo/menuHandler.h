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
void menuHandler();
static void back_event_handler(lv_event_t *e);
static void eventFunction1CB(lv_event_t *e);
static void eventFunction2CB(lv_event_t *e);
static lv_obj_t *sub1Function(lv_obj_t *menu);
static lv_obj_t *sub2Function(lv_obj_t *menu);
bool setupWifi();

/**********************
 *      MACROS
 **********************/

#endif /*MENU_HANDLER_H*/
