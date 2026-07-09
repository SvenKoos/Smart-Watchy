/**
 * @file deviceEvent.h
 *
 */

#ifndef DEVICE_EVENT_H
#define DEVICE_EVENT_H

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
static void device_event_cb(DeviceEvent_t event, void* params, void* user_data);
static void alertEventCB(lv_event_t * e);
lv_obj_t* prepareAlertScreen(int count);
void showAlert(singleAlert alert, int index, int count);
static void handle_button_emergency_reset();
void setupDeviceEvent();

/**********************
 *      MACROS
 **********************/

#endif /*DEVICE_EVENT_H*/
