#include "BLE_Bond.h"

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

int bondStatus		= BOND_STATUS_UNDEFINED;
int bondErrorCode = -1;

class BLECustomServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { 
	bondStatus = BOND_STATUS_CONNECTED; 
	pServer->getAdvertising()->start();
  }

  void onDisconnect(BLEServer *pServer) { 
	bondStatus = BOND_STATUS_DISCONNECTED; 
  }
};

class bondCallback : public BLECharacteristicCallbacks {
public:
  bondCallback(BLE_Bond *ble) { 
	_p_ble = ble; 
  }
  
  BLE_Bond *_p_ble;
};

class MySecurity : public BLESecurityCallbacks {
  bool onConfirmPIN(uint32_t pin){
    return true;  
  }
  
  uint32_t onPassKeyRequest(){
    return 0;
  }

  void onPassKeyNotify(uint32_t pass_key){
  }
  
  bool onSecurityRequest(){
    return true;
  }

  // called with failed
  void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl){
    if(cmpl.success){
		bondStatus = BOND_STATUS_BONDED; 
    }
    else{
		bondStatus = BOND_STATUS_FAILED; 
    }
	bondErrorCode = cmpl.fail_reason;
  }
};

static void my_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
switch(event){
    case ESP_GAP_BLE_AUTH_CMPL_EVT:{											// last called
      BLEAddress address = BLEAddress(param->ble_security.auth_cmpl.bd_addr);
      BLEDevice::whiteListAdd(address);
	  bondStatus = BOND_STATUS_AUTHCOMPLETE; 
      break;
    }   
    case ESP_GAP_BLE_SEC_REQ_EVT:{
	  esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
	  bondStatus = BOND_STATUS_SECURITYEVT; 
      break;
    }     
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:{
	  bondStatus = BOND_STATUS_UPDATECONNPARAMS; 
      break;
    }     
  }
}

// never called
void gattcEventHandler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    if (event == ESP_GATTC_DISCONNECT_EVT) {
	  bondStatus = BOND_STATUS_GATTDISCONNECT; 
    } else
	if (event == ESP_GATTC_CONNECT_EVT) {
		esp_ble_set_encryption(param->connect.remote_bda, ESP_BLE_SEC_ENCRYPT);
	    bondStatus = BOND_STATUS_GATTCONNECT; 
	}
}

//
// Constructor
BLE_Bond::BLE_Bond(void) {}

//
// Destructor
BLE_Bond::~BLE_Bond(void) {}

//
// begin
bool BLE_Bond::begin(const char *localName = "Watchy") {
  BLEDevice::setCustomGattcHandler(gattcEventHandler);
  
  // Create the BLE Device
  BLEDevice::init(localName);
  BLEDevice::setCustomGapHandler(my_gap_event_handler);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  BLEDevice::setSecurityCallbacks(new MySecurity());
  
  pServer->setCallbacks(new BLECustomServerCallbacks());

  // Create the BLE Service
  pService       = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                     CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ   |
                     BLECharacteristic::PROPERTY_WRITE
                   );

  pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    pCharacteristic->setValue("Hello World says Watchy");
  
  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new bondCallback(this));

  // Start the service(s)
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x12);  // set value to 0x00 to not advertise this parameter
  pAdvertising->start();

  BLESecurity *pSecurity = new BLESecurity();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);  

  return true;
}

int BLE_Bond::updateStatus() { return bondStatus; }

