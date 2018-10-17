/*
 * BleApplications.c
 *
 * Copyright 2017, Brian Silverman
 */
#include <BLEApplications.h>
#include <stdio.h>
#include "assert.h"

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
    OnBleRecv(data, len);
    return 0;
}

//
// Send a packet over BLE.
//
// @param data  payload
// @param len   length of payload
//
// @return 0 on success, else error.  
//
// NOTE: the error CYBLE_ERROR_MEMORY_ALLOCATION_FAILED can often occur and
// the packet should just be retried.  However, it must be retried AFTER the
// next call to CyBle_ProcessEvents(), or it will still fail.
//
CYBLE_API_RESULT_T BleSend(
    uint8 * data,
    int len
    )
{
    return BleSendNotification(CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, data, len);
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

