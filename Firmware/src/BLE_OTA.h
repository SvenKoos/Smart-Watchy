#ifndef _BLE_OTA_H_
#define _BLE_OTA_H_

#include "Arduino.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "esp_ota_ops.h"

#include "config.h"

class BLE_OTA;

class BLE_OTA {
public:
  BLE_OTA(void);
  ~BLE_OTA(void);

  bool begin(const char *localName);
  int updateStatus();
  int howManyBytes();

private:
  String local_name;

  BLEServer *pServer = NULL;

  BLEService *pESPOTAService                 = NULL;
  BLECharacteristic *pESPOTAIdCharacteristic = NULL;

  BLEService *pService                            = NULL;
  BLECharacteristic *pVersionCharacteristic       = NULL;
  BLECharacteristic *pOtaCharacteristic           = NULL;
  BLECharacteristic *pWatchFaceNameCharacteristic = NULL;
};

#endif
