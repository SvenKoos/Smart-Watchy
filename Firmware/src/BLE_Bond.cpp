#include "BLE_Bond.h"

// #define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
// #define CHARACTERISTIC_UUID "abcd1234-5678-90ab-cdef-1234567890ab"

// #define UNLOCK_SERVICE_UUID        "12345678-1234-5678-1234-56789abcdef0"
// #define UNLOCK_CHARACTERISTIC_UUID "abcdef01-1234-5678-1234-56789abcdef0"

// b4. UUIDs
#define SERVICE_UUID        "180A" 
#define CHARACTERISTIC_UUID "2A29"

static int bondStatus		= BOND_STATUS_UNDEFINED;
// static NimBLEServer* pServer = nullptr;

class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
		if (connInfo.isBonded()) {
			Serial.println("bondBLE Bonded");
			bondStatus = BOND_STATUS_BONDED;
		} else {
			Serial.println("bondBLE Connected");
			bondStatus = BOND_STATUS_CONNECTED;
		}

		if (connInfo.isEncrypted()) {
			Serial.println("bondBLE Encrypted");
		} else {
			Serial.println("bondBLE Not Encrypted");
		}		
		
		// WICHTIG: Erzwinge das Bonding-Protokoll
        // Das triggert am Handy das "Koppeln"-Pop-up
        NimBLEDevice::startSecurity(connInfo.getConnHandle());		
		
		// b3. Wir erlauben dem Smartphone eine hohe Latenz (Slave Latency).
        // Das bedeutet: Der ESP32 muss nicht auf jedes Paket antworten, 
        // hält die Verbindung aber logisch offen.
        // pServer->updateConnParams(connInfo.getConnHandle(), 100, 160, 4, 600);
        // Interval: ~150ms-200ms, Latency: 4 (überspringt 4 Intervalle), Timeout: 6s		
		// b6. Timeout muss groß genug sein (hier 10s), damit Android nicht nervös wird.
		pServer->updateConnParams(connInfo.getConnHandle(), 160, 160, 9, 1000);
    }

    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
		Serial.println("bondBLE Disconnected");
		bondStatus = BOND_STATUS_DISCONNECTED; 

        // NimBLEDevice::startAdvertising();
		// b4. start Advertizing after disconnect
		NimBLEDevice::getAdvertising()->start();
    }

    void onAuthenticationComplete(NimBLEConnInfo& connInfo) override {
/*
        if(!connInfo.isEncrypted()) {
			bondStatus = BOND_STATUS_FAILED; 
            pServer->disconnect(connInfo.getConnHandle());
        } else     
			if (connInfo.isBonded()) {
				bondStatus = BOND_STATUS_BONDED;
			} else {
				bondStatus = BOND_STATUS_CONNECTED;
			}
*/
		if (!connInfo.isEncrypted()) {
			Serial.println("bondBLE Failed");
			bondStatus = BOND_STATUS_FAILED;
		} else
		{
			// Bonding erfolgreich, auch wenn isBonded() noch false ist
			Serial.println("bondBLE Bonded");
			bondStatus = BOND_STATUS_BONDED;
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
	
	// b1. Security setzen (Wichtig für Extended Unlock!)
	NimBLEDevice::setSecurityAuth(true, true, true); // Bonding, MITM, Secure Connections
	NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT); // "Just Works" oder Passkey je nach Wunsch
	
    NimBLEServer* pServer = NimBLEDevice::createServer();
	pServer->setCallbacks(new ServerCallbacks());
	
	// b2. Minimalen Service erstellen
	NimBLEService *pService = pServer->createService(SERVICE_UUID);
	NimBLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                               CHARACTERISTIC_UUID,
                                               NIMBLE_PROPERTY::READ | 
                                               NIMBLE_PROPERTY::READ_ENC | // Verschlüsselung triggern
                                               NIMBLE_PROPERTY::READ_AUTHEN
                                             );
	pCharacteristic->setValue("ESP32-Auth");
	pService->start();	

    // Wir erstellen einen Standard-Service (Battery Service 0x180F)
    // Das signalisiert Android: "Ich bin ein nützliches Gerät"
    // pServer->createService(NimBLEUUID((uint16_t)0x1812));
	// pServer->createService(NimBLEUUID((uint16_t)0x180F));
	pServer->createService(NimBLEUUID((uint16_t)0x1812));
	
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();

    // WICHTIG: Alles in EIN Paket, falls Android Scan Responses ignoriert
    NimBLEAdvertisementData advData;
    
    // a1. Flags (Zwingend)
    // advData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
	advData.setFlags(0x06);
    
    // a2. Appearance (0x0200 = Generic Tag / 0x0080 = Computer)
    // advData.setAppearance(0x0200); 
	// advData.setAppearance(0x03C1);
	// advData.setAppearance(0x0200); // funktioniert
	advData.setAppearance(0x00C1);
	
	// a3. Manufacturer Data (Wir nutzen die Microsoft ID 0x0006 als "Tarnung")
    // Das zwingt den Samsung-Scanner oft dazu, das Gerät zu listen
    // std::string mntData = "\x06\x00\x01\x02\x03\x04"; 
    // advData.setManufacturerData(mntData);
    
    // a3. Service UUID (Battery Service ist sehr "unverfänglich")
    // advData.addServiceUUID(NimBLEUUID((uint16_t)0x180F));
	advData.addServiceUUID(NimBLEUUID((uint16_t)0x1812));
	
    // a4. Name (Kurz halten!)
    advData.setName(localName);
    pAdvertising->setAdvertisementData(advData);
    
    // Full Power für maximale Sichtbarkeit
    pAdvertising->start();

    return true;
}

int BLE_Bond::updateStatus() { return bondStatus; }

