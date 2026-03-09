#include "BLE_Bond.h"

// #define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
// #define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

#define UNLOCK_SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
#define UNLOCK_CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"

volatile int bondStatus		= BOND_STATUS_UNDEFINED;
// int bondErrorCode = -1;
static bool deviceConnected = false;
static NimBLEServer* pServer = nullptr;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
		bondStatus = BOND_STATUS_CONNECTED; 
		deviceConnected = true;
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
		bondStatus = BOND_STATUS_DISCONNECTED; 
		deviceConnected = false;		

        NimBLEDevice::startAdvertising();
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
        if(!connInfo.isEncrypted()) {
			bondStatus = BOND_STATUS_FAILED; 
            pServer->disconnect(connInfo.getConnHandle());
        } else     
			if (connInfo.isBonded()) {
				bondStatus = BOND_STATUS_BONDED;
			} else {
				bondStatus = BOND_STATUS_CONNECTED;
			}
    }
};

// Constructor
BLE_Bond::BLE_Bond(void) {}

// Destructor
BLE_Bond::~BLE_Bond(void) {}

/*
// begin
bool BLE_Bond::begin(const char *localName = "WatchyUnlock") {
  // BLEDevice::setCustomGattcHandler(gattcEventHandler);
  
  // Create the BLE Device
  NimBLEDevice::init(localName);
  // BLEDevice::setCustomGapHandler(my_gap_event_handler);

  //1 NimBLESecurity *pSecurity = new NimBLESecurity();
  // pSecurity->setAuthenticationMode(ESP_LE_AUTH_REQ_SC_BOND);
  //1 pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);
  //1 pSecurity->setCapability(ESP_IO_CAP_NONE);
  // uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
  // esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
  //1 pSecurity->setInitEncryptionKey(ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK);  
  
  NimBLEDevice::setSecurityAuth(true, true, true);
  // NimBLEDevice::setSecurityPasskey(0);  // optional
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityInitKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);
  NimBLEDevice::setSecurityRespKey(BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID);

  // Create the BLE Server
  pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  
  // BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
  // BLEDevice::setSecurityCallbacks(new MySecurity());
  
  // pServer->setCallbacks(new BLECustomServerCallbacks());

  // Create the BLE Service
  // pService       = pServer->createService(SERVICE_UUID);
  // NimBLEService* pService = pServer->createService(SERVICE_UUID);
  
  // Unlock characteristics
  // NimBLEService* pUnlockService = pServer->createService(UNLOCK_SERVICE_UUID);
  NimBLEService* pUnlockService = pServer->createService(NimBLEUUID((uint16_t)0x180F));
  NimBLECharacteristic* pUnlockChar = pUnlockService->createCharacteristic(
    UNLOCK_CHARACTERISTIC_UUID,
    NIMBLE_PROPERTY::READ_ENC);
  // statischer Wert – reicht für Trusted Device
  pUnlockChar->setValue("UNLOCK_OK");
  // Service starten
  pUnlockService->start();
  
  NimBLEAdvertising* pAdvertisingUnlock = NimBLEDevice::getAdvertising();
  // pAdvertisingUnlock->addServiceUUID(UNLOCK_SERVICE_UUID);
  pAdvertisingUnlock->addServiceUUID(NimBLEUUID((uint16_t)0x180F));
  
  // set device name
  NimBLEAdvertisementData advData;
  advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  // advData.setName(localName);  // ← Name ins Advertising-Paket

  // WICHTIG: Appearance setzen (z.B. Generic Display oder Computer)
  // 0x0000 = Unknown, 0x0080 = Generic Computer
  advData.setAppearance(0x0080);

  // advData.setCompleteServices(NimBLEUUID(UNLOCK_SERVICE_UUID));
  advData.setCompleteServices(NimBLEUUID(NimBLEUUID((uint16_t)0x180F)));
  pAdvertisingUnlock->setAdvertisementData(advData);

  NimBLEAdvertisementData scanData;
  scanData.setName(localName);
  pAdvertisingUnlock->setScanResponseData(scanData);
  
  pAdvertisingUnlock->start();

  return true;
}
*/

bool BLE_Bond::begin(const char *localName = "WatchyUnlock") {
NimBLEDevice::init(localName);
    NimBLEDevice::init(localName);
	
    NimBLEServer* pServer = NimBLEDevice::createServer();
    // Wir erstellen einen Standard-Service (Battery Service 0x180F)
    // Das signalisiert Android: "Ich bin ein nützliches Gerät"
    // pServer->createService(NimBLEUUID((uint16_t)0x1812));
	// pServer->createService(NimBLEUUID((uint16_t)0x180F));
	pServer->createService(NimBLEUUID((uint16_t)0x1812));
	
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    // WICHTIG: Alles in EIN Paket, falls Android Scan Responses ignoriert
    NimBLEAdvertisementData advData;
    
    // 1. Flags (Zwingend)
    // advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
	advData.setFlags(0x06);
    
    // 2. Appearance (0x0200 = Generic Tag / 0x0080 = Computer)
    // advData.setAppearance(0x0200); 
	// advData.setAppearance(0x03C1);
	advData.setAppearance(0x0200);
	
	// 3. Manufacturer Data (Wir nutzen die Microsoft ID 0x0006 als "Tarnung")
    // Das zwingt den Samsung-Scanner oft dazu, das Gerät zu listen
    // std::string mntData = "\x06\x00\x01\x02\x03\x04"; 
    // advData.setManufacturerData(mntData);
    
    // 3. Service UUID (Battery Service ist sehr "unverfänglich")
    // advData.addServiceUUID(NimBLEUUID((uint16_t)0x180F));
	advData.addServiceUUID(NimBLEUUID((uint16_t)0x1812));
	
    // 4. Name (Kurz halten!)
    advData.setName(localName);
    pAdvertising->setAdvertisementData(advData);
    
    // Full Power für maximale Sichtbarkeit
    pAdvertising->start();

    return true;
}

int BLE_Bond::updateStatus() { return bondStatus; }

void BLEAdvertise() {
  // Hier könntest du z.B. alle 60s einen Notify schicken, wenn verbunden
  static unsigned long lastNotify = 0;
  if (deviceConnected) {
    unsigned long now = millis();
    if (now - lastNotify > 59000) { // 60 Sekunden
      lastNotify = now;
      // Beispiel: Notify senden

    }
  }
}
