/*
 * BleApplications.c
 *
 * Copyright 2017, Brian Silverman
 */
#include <BLEApplications.h>
#include <stdio.h>

uint16 negotiatedMtu = DEFAULT_MTU_SIZE;

#define MAX_WRITE_CALLBACKS (16)

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

CYBLE_API_RESULT_T BleRegisterWriteCallback(
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

                char s[32];
                sprintf(s, "%d, %d, %d, %d\r\n", attrHandle, val[0], len,
                cyBle_customs[serviceIndex].\
                    customServiceInfo[characteristicIndex].customServiceCharHandle);
                UART_UartPutString(s);

                if (attrHandle == cyBle_customs[serviceIndex].\
                    customServiceInfo[characteristicIndex].customServiceCharHandle)
                {
                    UART_UartPutString("Before callback\r\n");
                    writeCallbacks.table[i].callback(attrHandle, val, len);
                    UART_UartPutString("After callback\r\n");

                    // Write the characteristic back to the database
                    BleWriteCharacteristic(attrHandle, val, len);
                    UART_UartPutString("After write\r\n");
                }
            }

            CyBle_GattsWriteRsp(cyBle_connHandle);
            UART_UartPutString("After Rsp\r\n");
            break;

        case CYBLE_EVT_GATTS_XCNHG_MTU_REQ:
            negotiatedMtu = (((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu < CYBLE_GATT_MTU) ?
                            ((CYBLE_GATT_XCHG_MTU_PARAM_T *)eventParam)->mtu : CYBLE_GATT_MTU;
            break;
            

        default:
            break;
    }
}

CYBLE_API_RESULT_T BleStart()
{
    return CyBle_Start(CustomEventHandler);    
}

