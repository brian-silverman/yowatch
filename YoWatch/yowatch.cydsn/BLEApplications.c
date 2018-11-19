/*
 * BleApplications.c
 *
 * Copyright 2017, Brian Silverman
 */
#include <BLEApplications.h>
#include <stdio.h>
#include <errno.h>
#include "assert.h"
#include "bleapi.h"
#include "tick.h"

int deviceConnected = 0;

static uint16 bleCmdIndex[BLE_API_MAX_CMD_NUM];
static struct {
    struct BLE_HEADER header;
    struct BLE_HEADER * pHeader;
    uint8 * pPayload;
    int payloadLen;
} currentResponse;
static uint8 bleBuf[MAX_MTU_SIZE];

uint16 negotiatedMtu = DEFAULT_MTU_SIZE;

#define MAX_WRITE_CALLBACKS (16)

typedef CYBLE_API_RESULT_T 
    BLE_WRITE_CALLBACK_T(
        CYBLE_GATT_DB_ATTR_HANDLE_T handle,
        uint8 * data, 
        int len
    );

struct {
    struct {
        BLE_WRITE_CALLBACK_T * callback;
        int serviceIndex;
        int characteristicIndex;
    } table[MAX_WRITE_CALLBACKS];
    int len;
} writeCallbacks;

CYBLE_API_RESULT_T BleSendNotification(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data, 
    int len
    )
{
    CYBLE_GATTS_HANDLE_VALUE_NTF_T notification;    
    
    notification.attrHandle = handle;               
    notification.value.val = data;
    notification.value.len = len;
    
    return CyBle_GattsNotification(cyBle_connHandle, &notification);
}

static CYBLE_API_RESULT_T BleRegisterWriteCallback(
    BLE_WRITE_CALLBACK_T * callback,
    int serviceIndex,
    int characteristicIndex
    )
{
    if (writeCallbacks.len >= MAX_WRITE_CALLBACKS) return CYBLE_ERROR_INSUFFICIENT_RESOURCES;

    writeCallbacks.table[writeCallbacks.len].callback = callback;
    writeCallbacks.table[writeCallbacks.len].serviceIndex = serviceIndex;
    writeCallbacks.table[writeCallbacks.len].characteristicIndex = characteristicIndex;

    writeCallbacks.len++;

    return 0;
}

CYBLE_API_RESULT_T BleWriteCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data, 
    int len
    )
{
    CYBLE_GATT_HANDLE_VALUE_PAIR_T characterisitc;  

    characterisitc.attrHandle = handle;
    characterisitc.value.val = data;
    characterisitc.value.len = len;
    //characterisitc.value.actualLen = len;

    return CyBle_GattsWriteAttributeValue(&characterisitc, 0, &cyBle_connHandle, 0);  
}

CYBLE_API_RESULT_T BleReadCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data, 
    int len
    )
{
    // TBD
    return -1;
}

//
// BLE callback when connection changed
//
static void OnConnectionChange(
    int connected
    )
{
    //xprintf("%s\r\n", connected ? "CONNECTED" : "DISCONNECTED");
    deviceConnected = connected;
}

int BleIsConnected(
    )
{
    return deviceConnected;
}

/*! 
 * Custom BLE event handler
 *
 * @param event         event that needs to be processed
 * @param eventParam    additional event info, event specific
 */
void CustomEventHandler(uint32 event, void * eventParam)
{
    CYBLE_GATTS_WRITE_REQ_PARAM_T * wrReqParam;
    int i;
   
    switch(event)
    {
        case CYBLE_EVT_STACK_ON:
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
            // Start Advertisement and enter Discoverable mode
            negotiatedMtu = DEFAULT_MTU_SIZE;
            CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            break;
            
        case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
            // If disconnected, Start Advertisement and enter Discoverable mode
            if (CYBLE_STATE_DISCONNECTED == CyBle_GetState()) {
                CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            }
            break;
            
        case CYBLE_EVT_GATT_CONNECT_IND:
            OnConnectionChange(1);
            break;
        
        case CYBLE_EVT_GATT_DISCONNECT_IND:
            OnConnectionChange(0);
            break;
        
            
        case CYBLE_EVT_GATTS_WRITE_REQ:                             
            //
            // This event is received when Central device sends a Write command
            // on an Attribute.  
            //
            // For each matching characteristic handle that has been registered via
            // BleRegisterWriteCallback(), run the callback function.  
            //
            wrReqParam = (CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam;
            
            for (i = 0; i < writeCallbacks.len; i++) {
                int serviceIndex = writeCallbacks.table[i].serviceIndex;
                int characteristicIndex = writeCallbacks.table[i].characteristicIndex;

                CYBLE_GATT_DB_ATTR_HANDLE_T attrHandle = wrReqParam->handleValPair.attrHandle;
                uint8 * val = wrReqParam->handleValPair.value.val;
                int len = wrReqParam->handleValPair.value.len;

                if (attrHandle == cyBle_customs[serviceIndex].\
                    customServiceInfo[characteristicIndex].customServiceCharHandle)
                {
                    writeCallbacks.table[i].callback(attrHandle, val, len);

                    // Write the characteristic back to the database
                    BleWriteCharacteristic(attrHandle, val, len);
                }
            }

            CyBle_GattsWriteRsp(cyBle_connHandle);
            break;

        case CYBLE_EVT_GATTS_XCNHG_MTU_REQ:
            negotiatedMtu = (((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu < CYBLE_GATT_MTU) ?
                            ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu : CYBLE_GATT_MTU;
            break;
            

        default:
            break;
    }
}

static __inline__ CYBLE_API_RESULT_T _OnBleRecv(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data,
    int len
    )
{
    assert(len >= sizeof(struct BLE_HEADER));
    currentResponse.header = *((struct BLE_HEADER *) data);
    currentResponse.pHeader = &currentResponse.header;
    currentResponse.pPayload = data + sizeof(struct BLE_HEADER);
    assert(len == BLE_MAX_PAYLOAD + sizeof(struct BLE_HEADER));
    currentResponse.payloadLen = currentResponse.header.payloadLen;
    OnBleRecv(currentResponse.pPayload, currentResponse.payloadLen);
    return 0;
}

void BleRecvPayload(
    struct BLE_HEADER ** header,
    uint8 ** payload,
    int * len
    )
{
    if (header) *header = currentResponse.pHeader;
    if (payload) *payload = currentResponse.pPayload;
    if (len) *len = currentResponse.payloadLen;
}

//
// Send a packet over BLE, and async wait for response.
//
// @param cmd   BLE_W2P_* cmd to send
// @param state SM current state
// @param buf   payload.  Must be the buffer returned by BleGetBuf()
// @param len   length of payload
// @param waitForResponse   
//              If true, wait for response (see below)
//
// @return 0 on success, else must be retried until completion.
//
// Intended to be called from main SM.  Call must be retried until return value
// is 0 (success), but only retried once per SM loop, so that the
// CyBle_ProcessEvents() is called.  When it returns success, this indicates
// that the BLE packet was sent, and if waitForResponse is true, it also
// indicates that a response has been received.
//
// NOTE: the error CYBLE_ERROR_MEMORY_ALLOCATION_FAILED can often occur from
// BleSendNotification() and the packet should just be retried.  However, it
// must be retried AFTER the next call to CyBle_ProcessEvents(), or it will
// still fail.
//
// NOTE: Small packets have frequent timeout errors.  In long term testing,
// packets with /len/ less than or equal to 16 bytes failed to send around 6%
// of the time.  If 16 < len < ~40, packets failed around 0.1% of the time.
// Larger packets rarely failed (only one out of ~500k packets of size 40 to
// 500 failed).  The rountrip time to send packets does not (or barely)
// increases by packet size.  Therefore, we always send packets of the length
// BLE_MAX_PAYLOAD, and encode read length in the header.  When timeout errors
// do occur, we resend the packet.
//
int BleSendWait(
    int cmd,
    int state,
    uint8 * buf,
    int len,
    int waitForResponse
    )
{
    int ret = -EAGAIN;
    static enum {
        IDLE,
        SENDING,
        WAITING_FOR_RESPONSE
    } localState = IDLE;
    static struct {
        int cmd;
        int state;
        uint8 * buf;
        int len;
        int waitForResponse;
        struct BLE_HEADER * pHeader;
    } args;
    static int start = 0;
    static int numResends = 0;

    if (localState != IDLE) {
        assert(args.cmd == cmd);
        assert(args.state == state);
        assert(args.buf == buf);
        assert(args.len == len);
        assert(args.waitForResponse == waitForResponse);
    }

    switch (localState) {
        case IDLE:
            currentResponse.pHeader = NULL;
            currentResponse.pPayload = NULL;
            currentResponse.payloadLen = 0;

            args.cmd = cmd;
            args.state = state;
            args.buf = buf;
            args.len = len;
            args.waitForResponse = waitForResponse;

            args.pHeader = (struct BLE_HEADER *) (buf - sizeof(struct BLE_HEADER));
            memset(args.pHeader, 0, sizeof(struct BLE_HEADER));
            args.pHeader->cmd = cmd;
            args.pHeader->state = state;
            args.pHeader->requiresResponse = waitForResponse;
            assert(cmd > 0 && cmd <= BLE_API_MAX_CMD_NUM);
            args.pHeader->index = bleCmdIndex[cmd];
            bleCmdIndex[cmd]++;
            args.pHeader->payloadLen = len;

            numResends = 0;
            localState = SENDING;
            break;

        case SENDING:
            ret = BleSendNotification(
                CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE,
                (uint8 *) args.pHeader,
                sizeof(struct BLE_HEADER) + BLE_MAX_PAYLOAD
                );
            if (ret == 0) {
                start = GetTick();
                localState = args.waitForResponse ? WAITING_FOR_RESPONSE : IDLE;
            }
            break;

        case WAITING_FOR_RESPONSE:
            if (GetTick() - start > 2000) {
                numResends++;
                assert(numResends <= 2);
                localState = SENDING;
            } else if (currentResponse.pHeader) {
                assert(memcmp(currentResponse.pHeader, args.pHeader, sizeof(*args.pHeader)) == 0);
                localState = IDLE;
            }
            break;

        default:
            assert(0);
            break;
    }

    return localState != IDLE;
}

//
// Return the global BLE send buffer.  Buf size is BLE_MAX_PAYLOAD
//
uint8 * BleGetBuf(
    )
{
    return &bleBuf[sizeof(struct BLE_HEADER)];
}

void BleStart(
    )
{
    assert(CyBle_Start(CustomEventHandler) == CYBLE_ERROR_OK);
    BleRegisterWriteCallback(
        _OnBleRecv,
        CYBLE_SMARTWATCH_SERVICE_SERVICE_INDEX,
        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_INDEX
        );
}
