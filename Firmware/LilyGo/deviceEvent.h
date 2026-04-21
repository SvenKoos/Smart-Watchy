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
void device_event_cb(DeviceEvent_t event, void* params, void* user_data);
void alertEventCB(lv_event_t * e);

/**********************
 *      MACROS
 **********************/

#endif /*DEVICE_EVENT_H*/
