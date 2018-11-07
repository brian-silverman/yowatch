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
#define BLE_MAX_PAYLOAD                 (500)
#if BLE_MAX_PAYLOAD > 500
    // BLE_MAX_PAYLOAD must not be greater than MAX_MTU_SIZE minus the
    // BLE_HEADER size.  However, in testing, I /may/ have found that sizes
    // exactly at the MTU size failed.  Therefore the current max size add a
    // little extra margin.
    #error BLE_MAX_PAYLOAD too large
#endif

struct BLE_HEADER {
    uint16 cmd:5;
    uint16 state:5;
    uint16 requiresResponse:1;
    uint16 reserved:5;
    uint16 index;
};

void BleStart(
    );

void OnBleRecv(
    uint8 * data,
    int len
    );

int BleSendWait(
    int cmd,
    int state,
    uint8 * buf,
    int len,
    int waitForResponse
    );

uint8 * BleGetBuf(
    );

int BleIsConnected(
    );

void BleRecvPayload(
    struct BLE_HEADER ** header,
    uint8 ** payload,
    int * len
    );

#endif
