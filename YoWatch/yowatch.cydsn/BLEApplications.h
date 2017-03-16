#if !defined(_BLE_APPLICATIONS_H)
#define _BLE_APPLICATIONS_H

#include <project.h>

#define BLE_STATE_ADVERTISING			(0x01)
#define BLE_STATE_CONNECTED				(0x02)
#define BLE_STATE_DISCONNECTED			(0x00)

#define MTU_XCHANGE_DATA_LEN			(0x0020)
#define MAX_MTU_SIZE                    (512)
#define DEFAULT_MTU_SIZE                (CYBLE_GATT_MTU)


typedef CYBLE_API_RESULT_T 
    BLE_WRITE_CALLBACK_T(
        CYBLE_GATT_DB_ATTR_HANDLE_T handle,
        uint8 * data, 
        int len
    );
    
extern
CYBLE_API_RESULT_T BleWriteCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data, 
    int len
    );
extern
CYBLE_API_RESULT_T BleRegisterWriteCallback(
    BLE_WRITE_CALLBACK_T * callback,
    int serviceIndex,
    int characteristicIndex
    );
extern void OnConnectionChange(
    int connected
);
extern
CYBLE_API_RESULT_T BleSendNotification(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data, 
    int len
    );
extern
CYBLE_API_RESULT_T BleStart();

#endif
