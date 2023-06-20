#ifndef _BLE_BOND_H_
#define _BLE_BOND_H_

#include "Arduino.h"

#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "config.h"

#define BOND_STATUS_UNDEFINED    		-1
#define BOND_STATUS_CONNECTED    		0
#define BOND_STATUS_BONDED    			1
#define BOND_STATUS_FAILED    			2
#define BOND_STATUS_DISCONNECTED 		3
#define BOND_STATUS_GATTCONNECT 		4
#define BOND_STATUS_GATTDISCONNECT 		5
#define BOND_STATUS_AUTHCOMPLETE 		6
#define BOND_STATUS_SECURITYEVT 		7
#define BOND_STATUS_UPDATECONNPARAMS	8

class BLE_Bond;

class BLE_Bond {
public:
  BLE_Bond(void);
  ~BLE_Bond(void);

  bool begin(const char *localName);
  int updateStatus();

private:
  String local_name;

  BLEServer *pServer = NULL;
  BLECharacteristic* pCharacteristic = NULL;
  BLEService *pService                            = NULL;
};

#endif
