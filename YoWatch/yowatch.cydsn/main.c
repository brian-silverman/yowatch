/*
 * YoWatch main
 *
 * The YoWatch smartwatch runs on the PSoC 4 BLE (specifically the
 * CY8C4248LQI-BL583).  The watch is intended to be paired with a Android phone
 * (appropriately called YoPhone) - without one, it is virtually useless.  The
 * watch has 3 primary functions:
 *      1) Tell time - duh!
 *      2) Display notifications received in YoPhone (email, texts, etc)
 *      3) Fast answers - Alexa style response to questions
 *
 * As such, #3 is the bulk of the work in this app.  To support this, the
 * hardware contains:
 *      - A I2S Microphone
 *      - A Serial RAM for audio buffering
 *      - A BLE antenna (and BLE subsystem supported by the PSoC)
 *      - An OLED display
 *      - An accelerometer
 *
 * So, the normal use case to support #3 is:
 *      - Accelerometer twist or key press to turn on
 *      - Microphone begins recording audio, and buffering it
 *      - Audio is sent via BLE to YoPhone
 *      - YoPhone uses Speech-to-text cloud service
 *          - Cloud converts audio to text
 *          - YoPhone sends back partial text to YoWatch
 *          - YoWatch displays partial text when it receives it
 *      - YoPhone uses final text to run specific command.  
 *          - In general, the command does a web search for some data, parses
 *          the data, and sends the data back to YoWatch
 *          - YoPhone may use a combination of cloud services, like Google.
 *          - YoWatch displays the final data
 *      - Example:
 *          - User says "how many square miles in maryand"
 *              - YoWatch displays: "12,407 mi²"
 *          - User says "What is the capital of vermont"
 *              - YoWatch displays: "Montpelier"
 *          - User says "who played deckard in blade runner"
 *              - YoWatch displays: "Harrison Ford"
 *          - User says "text jason lets do lunch"
 *              - YoWatch sends a text to Jason
 *      - YoWatch is dumb, in that it doesn't understand the commands.  It just
 *      sends audio and receives text to display.  All the command types are
 *      handled by YoPhone.
 *
 * The flow of audio is:
 *
 *           DMA
 *  I2S Mic -----> buf1
 *                       DMA               DMA
 *                 buf2 -----> Serial RAM -----> buf -> compress -> BLE
 *
 *           (ping pong bufs)
 *
 * Copyright (C) 2017 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "BLEApplications.h"
#include "debug_locally.h"
#include "printf.h"
#include "post.h"
#include "assert.h"
#include "spi.h"
#include "queue.h"
#include "i2s.h"

//
// Speedtest buffer
// TODO: merge with other buffers?
//
uint8 buffer[MAX_MTU_SIZE+4];
int speedTestPackets;
int deviceConnected = 0;

//
// Main state machine STATEs
//
enum STATE {
    NONE,
    OFF,
    SLEEP,
    TIME,
    VOICE,
    NOTES,
    NOTE_VIEW,
    RUN_CMD,
    RESULT,
    CUSTOM_CMD,
    DEBUG_LOCALLY,
    DEBUG_IDLE,
    DEBUG_SPEEDTEST,
    DEBUG_SPEEDTEST_2,
    DEBUG_MIC,
    DEBUG_MIC_2,
    DEBUG_MIC_3,
    DEBUG_MIC_4,
} state, bleSelectedNextState;

void PrintState(
    enum STATE state
    )
{
    #define PRINT_CASE(state) case state: xprintf("State: " #state "\r\n"); break
    #define NO_PRINT_CASE(state) case state: break

    switch (state) {
        PRINT_CASE(NONE);
        PRINT_CASE(OFF);
        PRINT_CASE(SLEEP);
        PRINT_CASE(TIME);
        PRINT_CASE(VOICE);
        PRINT_CASE(NOTES);
        PRINT_CASE(NOTE_VIEW);
        PRINT_CASE(RUN_CMD);
        PRINT_CASE(RESULT);
        PRINT_CASE(CUSTOM_CMD);
        PRINT_CASE(DEBUG_LOCALLY);
        PRINT_CASE(DEBUG_IDLE);
        PRINT_CASE(DEBUG_SPEEDTEST);
        PRINT_CASE(DEBUG_SPEEDTEST_2);
        PRINT_CASE(DEBUG_MIC);
        NO_PRINT_CASE(DEBUG_MIC_2);
        NO_PRINT_CASE(DEBUG_MIC_3);
        NO_PRINT_CASE(DEBUG_MIC_4);
        default:
            xprintf("State: UNKNOWN!\r\n");
            break;
    }
}

//
// BLE callback when debug command received
//
CYBLE_API_RESULT_T OnDebugCommandCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data,
    int len
    )
{
    xprintf("%d, %d, %d\r\n", *data, len, state);

    // TODO Fix hardcoded constants
    switch (*data) {
        case 0: // Exit debug mode
            bleSelectedNextState = SLEEP;
            break;
        case 1: // Go to debug
            if (state == SLEEP) {
                bleSelectedNextState = DEBUG_IDLE;
            }
            xprintf("Go to DEBUG_IDLE\r\n");
            PrintState(state);
            break;
        case 2: // Go to debug speedtest
            speedTestPackets = 100; // TODO need correct # packets
            if (state == DEBUG_IDLE) {
                bleSelectedNextState = DEBUG_SPEEDTEST;
            }
            break;
        case 3:
            if (state == DEBUG_IDLE) {
                bleSelectedNextState = DEBUG_MIC;
            }
            break;
        default:
            bleSelectedNextState = NONE;
            break;
    }

    return 0;
}

//
// BLE callback when connection changed
// TODO handle state change (in main state machine) when device disconnects
//
void OnConnectionChange(
    int connected
    )
{
    uint8 c = 0;
    BleWriteCharacteristic(CYBLE_SMARTWATCH_SERVICE_VOICE_DATA_CHAR_HANDLE, &c, sizeof(c));
    deviceConnected = connected;
}

enum STATE GetBleStateIfNew(enum STATE state)
{
    if (bleSelectedNextState != NONE) {
        state = bleSelectedNextState;
    }
    bleSelectedNextState = NONE;
    return state;
}

//
// Main entry point:
//      - Init modules
//      - Run state machine forever
//
int main()
{
    uint32 n = 0;
    int ret;
    enum STATE newState, prevState;
    int done;
    int micBytes;

    prevState = OFF;
    state = DEBUG_MIC;

    CyDmaEnable();
    CyIntEnable(CYDMA_INTR_NUMBER);

    CyGlobalIntEnable;

    I2sRxDma_Init();
    SpiTxDma_Init();
    SpiRxDma_Init();

    BleStart();
    BleRegisterWriteCallback(
        OnDebugCommandCharacteristic,
        CYBLE_SMARTWATCH_SERVICE_SERVICE_INDEX,
        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_INDEX
        );

    I2S_1_Start();
    Timer_1_Start();
    SPI_1_Start();
    SpiTxIsr_StartEx(SpiTxIsr);
    UART_1_Start();

    Timer_Programmable_Init();

    SerialRamInit();
    BufQueueInit();

    Post();
    for(;;);

    for (;;) {
        CyBle_ProcessEvents();

        if (state != prevState) PrintState(state);

        newState = NONE;
        switch (state) {
            case OFF:
                newState = SLEEP;
                break;

            case SLEEP:
                newState = GetBleStateIfNew(newState);
                break;

            case TIME:
                break;

            case VOICE:
                break;

            case NOTES:
                break;

            case NOTE_VIEW:
                break;

            case RUN_CMD:
                break;

            case RESULT:
                break;

            case CUSTOM_CMD:
                break;

            case DEBUG_IDLE:
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_LOCALLY:
                if (DebugLocallySM()) newState = SLEEP;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_SPEEDTEST:
                if (speedTestPackets > 0) {
                    if(CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
                        ((uint32 *) buffer)[0] = 0;
                        buffer[0] = 3;
                        ((uint32 *) buffer)[1] = n++;
                        ((uint32 *) buffer)[2] = 0x11223344;

                        ret = BleSendNotification(
                            CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, buffer, 500);
                        if (ret == 0) {
                            speedTestPackets--;
                        }
                    }
                } else {
                    newState = DEBUG_SPEEDTEST_2;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_SPEEDTEST_2:
                if(CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
                    ((uint32 *) buffer)[0] = 0;
                    buffer[0] = 4;

                    ret = BleSendNotification(
                        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, buffer, 1);
                    if (ret == 0) {
                        newState = DEBUG_IDLE;
                    }
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MIC:
                BufQueueInit();
                micBytes = 0;
                I2sStartDma(NULL, NULL);
                newState = DEBUG_MIC_2;
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MIC_2:
                ret = DequeueBytes(buffer, 496, &done);
                if (ret != -ENODATA) {
                    newState = DEBUG_MIC_3;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MIC_3:
                if (done) {
                    newState = DEBUG_MIC_4;
                }
                newState = GetBleStateIfNew(newState);
                break;

            case DEBUG_MIC_4:
                if(CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
                    buffer[0] = 5;
                    ret = BleSendNotification(
                        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_HANDLE, buffer, 500);
                    if (ret == 0) {
                        micBytes += 500;
                        newState = DEBUG_MIC_2;
                    }
                }
                if (micBytes > 100000) {
                    I2sStopDma();
                    newState = DEBUG_IDLE;
                }
                newState = GetBleStateIfNew(newState);
                break;

            default:
                break;
        }

        prevState = state;
        if (newState != NONE) {
            state = newState;
        }
    }
}
