/**
 * @file ble.h
 *
 */

#ifndef BLE_H
#define BLE_H

/*********************
 *      INCLUDES
 *********************/

/*********************
 *      DEFINES
 *********************/
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

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 * GLOBAL PROTOTYPES
 **********************/
class BLE_Bond;

class BLE_Bond {
  public:
    BLE_Bond(void);
    ~BLE_Bond(void);

    bool begin(const char *localName);
    int updateStatus();

  private:
    String local_name;
};

void stopBLE();
void startBLE(BLE_Bond BT);
int bondBLE();

/**********************
 *      MACROS
 **********************/

#endif /*BLE_H*/
