#if !defined(_BLE_APPLICATIONS_H)
#define _BLE_APPLICATIONS_H

#include <project.h>
#include <BLEApplications.h>

#define BLE_STATE_ADVERTISING			(0x01)
#define BLE_STATE_CONNECTED				(0x02)
#define BLE_STATE_DISCONNECTED			(0x00)

#define MTU_XCHANGE_DATA_LEN			(0x0020)
#define MAX_MTU_SIZE                    (512)
#define DEFAULT_MTU_SIZE                (CYBLE_GATT_MTU)

void OnConnectionChange(
    int connected
);

void BleStart(
    );

void OnBleRecv(
    uint8 * data,
    int len
    );

CYBLE_API_RESULT_T BleSend(
    uint8 * data,
    int len
    );

#endif
