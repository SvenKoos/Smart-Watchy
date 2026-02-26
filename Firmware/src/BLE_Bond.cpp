#include "BLE_Bond.h"

#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

int bondStatus		= BOND_STATUS_UNDEFINED;
int bondErrorCode = -1;

static bool deviceConnected = false;
static BLEServer* pServer = nullptr;

class BLECustomServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) { 
	bondStatus = BOND_STATUS_CONNECTED; 
	deviceConnected = true;
	pServer->getAdvertising()->start();
	// BLEDevice::startAdvertising();
  }

  void onDisconnect(BLEServer *pServer) { 
	bondStatus = BOND_STATUS_DISCONNECTED; 
    deviceConnected = false;
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
bool BLE_Bond::begin(const char *localName = "WatchyUnlock") {
  BLEDevice::setCustomGattcHandler(gattcEventHandler);
  
  // Create the BLE Device
  BLEDevice::init(localName);
  // BLEDevice::setCustomGapHandler(my_gap_event_handler);

  BLESecurity *pSecurity = new BLESecurity();
  // pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  pSecurity->setCapability(ESP_IO_CAP_NONE);
  // uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  // esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);  

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  
  // BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  // BLEDevice::setSecurityCallbacks(new MySecurity());
  
  pServer->setCallbacks(new BLECustomServerCallbacks());

  // Create the BLE Service
  pService       = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristic = pService->createCharacteristic(
                     CHARACTERISTIC_UUID,
                     BLECharacteristic::PROPERTY_READ   |
                     BLECharacteristic::PROPERTY_WRITE
                   );

  // pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
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
  // BLEDevice::startAdvertising();

  return true;
}

/*
static BLEServer* pServer = nullptr;
static bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) override {
    deviceConnected = true;
	bondStatus = BOND_STATUS_CONNECTED;
    Serial.println("BLE: Client connected");
  }

  void onDisconnect(BLEServer* pServer) override {
    deviceConnected = false;
	bondStatus = BOND_STATUS_DISCONNECTED; 
    Serial.println("BLE: Client disconnected");
    // Nach Disconnect wieder advertiset, damit Android neu verbinden kann
    BLEDevice::startAdvertising();
  }
};

bool BLE_Bond::begin(const char *localName = "WatchyUnlock") {
  Serial.begin(115200);

  BLEDevice::init(localName);  // Name, der auf Android angezeigt wird

  // Security konfigurieren (Bonding)
  BLESecurity* pSecurity = new BLESecurity();
  // SvKo changed
  // pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND); // Bonding aktiv
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND); // Bonding aktiv
  // SvKo changed
  // pSecurity->setCapability(ESP_IO_CAP_OUT);           // je nach gewünschter Methode
  pSecurity->setCapability(ESP_IO_CAP_NONE);           // je nach gewünschter Methode
  pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);
  // SvKo added
  uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  // SvKo added
  BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);

  // Einfacher Service + Characteristic (für Debug/Keep-Alive)
  BLEService* pService = pServer->createService(SERVICE_UUID);

  BLECharacteristic* pCharacteristic = pService->createCharacteristic(
      CHARACTERISTIC_UUID,
      BLECharacteristic::PROPERTY_READ | 
// SvKo changed
//      BLECharacteristic::PROPERTY_NOTIFY
      BLECharacteristic::PROPERTY_WRITE
  );

  // SvKo added
  pCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

  pCharacteristic->setValue("Hello Android");
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();

  // Advertising konfigurieren
  BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(pService->getUUID());
  pAdvertising->setScanResponse(true);
  // SvKo removed
  // pAdvertising->setMinPreferred(0x06);  // optional
  pAdvertising->setMinPreferred(0x12);  // optional

  // SvKo changed
  // BLEDevice::startAdvertising();
  pAdvertising->start();

  Serial.println("BLE: Advertising started");

  return true;
}
*/

int BLE_Bond::updateStatus() { return bondStatus; }

void BLEAdvertise() {
  // Hier könntest du z.B. alle 60s einen Notify schicken, wenn verbunden
  static unsigned long lastNotify = 0;
  if (deviceConnected) {
    unsigned long now = millis();
    if (now - lastNotify > 59000) { // 60 Sekunden
      lastNotify = now;
      // Beispiel: Notify senden
      BLEService* pService = pServer->getServiceByUUID(SERVICE_UUID);
      if (pService) {
        BLECharacteristic* pChar = pService->getCharacteristic(CHARACTERISTIC_UUID);
        if (pChar) {
          pChar->setValue("Ping");
          pChar->notify();
          Serial.println("BLE: Notify Ping");
        }
      }
    }
  }
}
