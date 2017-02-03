/*****************************************************************************
* File Name: BleApplications.c
*
* Version: 1.0
*
* Description:
* This file implements the BLE capability.
*
* Hardware Dependency:
* CY8CKIT-042 BLE Pioneer Kit
*
******************************************************************************
* Copyright (2014), Cypress Semiconductor Corporation.
******************************************************************************
* This software is owned by Cypress Semiconductor Corporation (Cypress) and is
* protected by and subject to worldwide patent protection (United States and
* foreign), United States copyright laws and international treaty provisions.
* Cypress hereby grants to licensee a personal, non-exclusive, non-transferable
* license to copy, use, modify, create derivative works of, and compile the
* Cypress Source Code and derivative works for the sole purpose of creating
* custom software in support of licensee product to be used only in conjunction
* with a Cypress integrated circuit as specified in the applicable agreement.
* Any reproduction, modification, translation, compilation, or representation of
* this software except as specified above is prohibited without the express
* written permission of Cypress.
*
* Disclaimer: CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH
* REGARD TO THIS MATERIAL, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
* Cypress reserves the right to make changes without further notice to the
* materials described herein. Cypress does not assume any liability arising out
* of the application or use of any product or circuit described herein. Cypress
* does not authorize its products for use as critical components in life-support
* systems where a malfunction or failure may reasonably be expected to result in
* significant injury to the user. The inclusion of Cypress' product in a life-
* support systems application implies that the manufacturer assumes all risk of
* such use and in doing so indemnifies Cypress against all charges. Use may be
* limited by and subject to the applicable Cypress software license agreement.
*****************************************************************************/

/*****************************************************************************
* Included headers
*****************************************************************************/
#include <main.h>
#include <BLEApplications.h>

/*****************************************************************************
* Static variables 
*****************************************************************************/

/* 'rgbHandle' stores RGB control data parameters */
CYBLE_GATT_HANDLE_VALUE_PAIR_T		rgbHandle;	

/* Array to store the present RGB LED control data. The 4 bytes 
* of the array represents {R, G, B, Intensity} */
uint8 RGBledData[RGB_CHAR_DATA_LEN];

/* This flag is used by application to know whether a Central 
* device has been connected. This is updated in BLE event callback 
* function*/
uint8 deviceConnected = FALSE;


/*******************************************************************************
* Function Name: CustomEventHandler
********************************************************************************
* Summary:
* Call back event function to handle various events from BLE stack
*
* Parameters:
*  event:		event returned
*  eventParam:	link to value of the events returned
*
* Return:
*  void
*
*******************************************************************************/
void CustomEventHandler(uint32 event, void * eventParam)
{
	CYBLE_GATTS_WRITE_REQ_PARAM_T *wrReqParam;
   
    switch(event)
    {
        case CYBLE_EVT_STACK_ON:
        case CYBLE_EVT_GAP_DEVICE_DISCONNECTED:
			/* Start Advertisement and enter Discoverable mode*/
			CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
			break;
			
		case CYBLE_EVT_GAPP_ADVERTISEMENT_START_STOP:
			/* Set the BLE state variable to control LED status */
            if(CYBLE_STATE_DISCONNECTED == CyBle_GetState())
            {
                /* Start Advertisement and enter Discoverable mode*/
                CyBle_GappStartAdvertisement(CYBLE_ADVERTISING_FAST);
            }
			break;
			
        case CYBLE_EVT_GATT_CONNECT_IND:
			/* This flag is used in application to check connection status */
			deviceConnected = TRUE;
			break;
        
        case CYBLE_EVT_GATT_DISCONNECT_IND:
			/* Update deviceConnected flag*/
			deviceConnected = FALSE;
			
            /* Reset the color coordinates */
			RGBledData[RED_INDEX] = ZERO;
            RGBledData[GREEN_INDEX] = ZERO;
            RGBledData[BLUE_INDEX] = ZERO;
            RGBledData[INTENSITY_INDEX] = ZERO;
			UpdateRGBled();

			break;
        
            
        case CYBLE_EVT_GATTS_WRITE_REQ: 							
            /* This event is received when Central device sends a Write command 
             * on an Attribute. 
             * We first get the attribute handle from the event parameter and 
             * then try to match that handle with an attribute in the database.
             */
            wrReqParam = (CYBLE_GATTS_WRITE_REQ_PARAM_T *) eventParam;
            

            /* This condition checks whether the RGB LED characteristic was
             * written to by matching the attribute handle.
             * If the attribute handle matches, then the value written to the 
             * attribute is extracted and used to drive RGB LED.
             */
            
            /* ADD_CODE to extract the attribute handle for the RGB LED 
             * characteristic from the custom service data structure.
             */
            if(wrReqParam->handleValPair.attrHandle == cyBle_customs[RGB_LED_SERVICE_INDEX].\
								customServiceInfo[RGB_LED_CHAR_INDEX].customServiceCharHandle)
            {
                /* ADD_CODE to extract the value of the attribute from 
                 * the handle-value pair database. */
                RGBledData[RED_INDEX] = wrReqParam->handleValPair.value.val[RED_INDEX];
                RGBledData[GREEN_INDEX] = wrReqParam->handleValPair.value.val[GREEN_INDEX];
                RGBledData[BLUE_INDEX] = wrReqParam->handleValPair.value.val[BLUE_INDEX];
                RGBledData[INTENSITY_INDEX] = wrReqParam->handleValPair.value.val[INTENSITY_INDEX];
                
                /* Update the PrISM components and the attribute for RGB LED read 
                 * characteristics */
                UpdateRGBled();
            }

            
			/* ADD_CODE to send the response to the write request received. */
			CyBle_GattsWriteRsp(cyBle_connHandle);
			
			break;

        default:

       	 	break;
    }
}


/*******************************************************************************
* Function Name: SendCapSenseNotification
********************************************************************************
* Summary:
* Send CapSense Slider data as BLE Notifications. This function updates
* the notification handle with data and triggers the BLE component to send 
* notification
*
* Parameters:
*  CapSenseSliderData:	CapSense slider value	
*
* Return:
*  void
*
*******************************************************************************/
void SendCapSenseNotification(uint8 CapSenseSliderData)
{
	/* 'CapSensenotificationHandle' stores CapSense notification data parameters */
	CYBLE_GATTS_HANDLE_VALUE_NTF_T		CapSensenotificationHandle;	
	
	/* Update notification handle with CapSense slider data*/
	CapSensenotificationHandle.attrHandle = CAPSENSE_SLIDER_CHAR_HANDLE;				
	CapSensenotificationHandle.value.val = &CapSenseSliderData;
	CapSensenotificationHandle.value.len = CAPSENSE_CHAR_DATA_LEN;
	
	/* Send notifications. */
	CyBle_GattsNotification(cyBle_connHandle, &CapSensenotificationHandle);
}


/*******************************************************************************
* Function Name: UpdateRGBled
********************************************************************************
* Summary:
* Receive the new RGB data and modify PrISM parameters. Also, update the
* read characteristic handle so that the next read from the BLE central device
* gives present RGB color and intensity data.
*
* Parameters:
*  void
*
* Return:
*  void
*
*******************************************************************************/
void UpdateRGBled(void)
{
	/* Local variables to calculate the color components from RGB received data*/
	uint8 debug_red;
	uint8 debug_green;
	uint8 debug_blue;
	uint8 intensity_divide_value = RGBledData[INTENSITY_INDEX];
	
	debug_red = (uint8)(((uint16)RGBledData[RED_INDEX] * intensity_divide_value) / 255);
	debug_green = (uint8)(((uint16)RGBledData[GREEN_INDEX] * intensity_divide_value) / 255);
	debug_blue = (uint8)(((uint16)RGBledData[BLUE_INDEX] * intensity_divide_value) / 255);
	
	/* Update the density value of the PrISM module for color control*/
	PRS_1_WritePulse0(RGB_LED_MAX_VAL - debug_red);
    PRS_1_WritePulse1(RGB_LED_MAX_VAL - debug_green);
    PRS_2_WritePulse0(RGB_LED_MAX_VAL - debug_blue);
	
	/* Update RGB control handle with new values */
	rgbHandle.attrHandle = RGB_LED_CHAR_HANDLE;
	rgbHandle.value.val = RGBledData;
	rgbHandle.value.len = RGB_CHAR_DATA_LEN;
	rgbHandle.value.actualLen = RGB_CHAR_DATA_LEN;
	
	/* Send updated RGB control handle as attribute for read by central device */
	CyBle_GattsWriteAttributeValue(&rgbHandle, FALSE, &cyBle_connHandle, FALSE);  
}


/* [] END OF FILE */
