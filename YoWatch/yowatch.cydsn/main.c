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
#include "oled.h"

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
    MSGS,
    MSG_VIEW,
    RUN_CMD,
    RESULT,
    CUSTOM_CMD,
    SPEED_TEST,
    MIC_TEST,
    DISCONNECT,
    MAX_STATES
};

enum {
    FIRST_STATE_CALL,
    MIDDLE_STATE_CALL,
    LAST_STATE_CALL
};

//
// Transitions
//
#define BLE_DISCONNECT      (1 << 0)
#define BLE_CONNECT         (1 << 1)
#define BLE_CUSTOM_CMD      (1 << 2)
#define BLE_RESULT          (1 << 3)
#define BLE_MESSAGE         (1 << 4)
#define BLE_MIC_TEST        (1 << 5)
#define BLE_SPEED_TEST      (1 << 6)
#define BLE_END_TEST        (1 << 7)

#define BUTTON_FORWARD      (1 << 0)
#define BUTTON_BACK         (1 << 1)
#define BUTTON_UP           (1 << 2)
#define BUTTON_DOWN         (1 << 3)
#define BUTTON_ANY          (BUTTON_FORWARD | BUTTON_BACK | BUTTON_UP | BUTTON_DOWN)
#define BUTTON_LEFT         BUTTON_BACK          
#define BUTTON_RIGHT        BUTTON_FORWARD          

#define ACCEL_TWIST         (1 << 0)

#define VOICE_ACTIVITY      (1 << 0)
#define VOICE_TIMEOUT       (1 << 1)


//
// BLE callback when debug command received
//
CYBLE_API_RESULT_T OnDebugCommandCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data,
    int len
    )
{
#if 0
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
#endif

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
#if 0
    if (bleSelectedNextState != NONE) {
        state = bleSelectedNextState;
    }
    bleSelectedNextState = NONE;
#endif
    return state;
}

//////////////////////////////////////////////////////////////////////
//
// State transition functions
//

int TrAccel(
    )
{
    return 0;
}

int TrBle(
    )
{
    return 0;
}

int TrButton(
    )
{
    return 0;
}

int TrGoToSleep(
    )
{
    return 0;
}

int TrVoice(
    )
{
    return 1;
}

//////////////////////////////////////////////////////////////////////
//
// State functions
//

void SmOff(
    int prevState,
    int call
    )
{
    DisplayInit();
    SerialRamInit();
    BufQueueInit();
}

void SmSleep(
    int prevState,
    int call
    )
{
}

void SmTime(
    int prevState,
    int call
    )
{
    if (call == FIRST_STATE_CALL) {
        Post();
    }
}

void SmVoice(
    int prevState,
    int call
    )
{
}

void SmMsgs(
    int prevState,
    int call
    )
{
}

void SmMsgView(
    int prevState,
    int call
    )
{
}

void SmRunCmd(
    int prevState,
    int call
    )
{
    xprintf("HALT!\r\n");
    for(;;);
}

void SmResult(
    int prevState,
    int call
    )
{
}

void SmCustomCmd(
    int prevState,
    int call
    )
{
}

void SmSpeedTest(
    int prevState,
    int call
    )
{
#if 0
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
#endif
}

void SmMicTest(
    int prevState,
    int call
    )
{
#if 0
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
#endif
}

void SmDisconnect(
    int prevState,
    int call
    )
{
}

//
// State Machine transition structure
//
#define NAME(n) n, #n
#define MAX_STATE_TRANSITIONS 5
const struct {
    int state;
    char * name;
    void (*func)(int, int);
    struct TRANSITION {
        int (*condition)(int);
        int condArg;
        int newState;
    } transition[MAX_STATE_TRANSITIONS];
} SM[MAX_STATES] = {
    { NAME(NONE) },
    { NAME(OFF), SmOff, {
        { NULL, 0, TIME }
        }},
    { NAME(SLEEP), SmSleep, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrAccel, ACCEL_TWIST, TIME },
        { TrButton, BUTTON_ANY, TIME },
        { TrBle, BLE_SPEED_TEST | BLE_MIC_TEST, DISCONNECT },
        }},
    { NAME(TIME), SmTime, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrVoice, VOICE_ACTIVITY, VOICE },
        { TrButton, BUTTON_FORWARD, MSGS },
        }},
    { NAME(VOICE), SmVoice, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrVoice, VOICE_TIMEOUT, RUN_CMD },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(MSGS), SmMsgs, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrButton, BUTTON_UP | BUTTON_DOWN, MSGS },
        { TrButton, BUTTON_FORWARD, MSG_VIEW },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(MSG_VIEW), SmMsgView, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrButton, BUTTON_UP | BUTTON_DOWN, MSG_VIEW },
        { TrButton, BUTTON_BACK, MSGS },
        }},
    { NAME(RUN_CMD), SmRunCmd, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrBle, BLE_CUSTOM_CMD, CUSTOM_CMD },
        { TrBle, BLE_RESULT, RESULT },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(RESULT), SmResult, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrButton, BUTTON_UP | BUTTON_DOWN, RESULT },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(CUSTOM_CMD), SmCustomCmd, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrButton, BUTTON_UP | BUTTON_DOWN | BUTTON_FORWARD, CUSTOM_CMD },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(SPEED_TEST), SmSpeedTest, {
        { TrBle, BLE_END_TEST, SLEEP },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(MIC_TEST), SmMicTest, {
        { TrBle, BLE_END_TEST, SLEEP },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(DISCONNECT), SmDisconnect, {
        { TrBle, BLE_CONNECT, TIME },
        }},
};


//
// Main entry point:
//      - Init modules
//      - Run state machine forever
//
int main()
{
    enum STATE state, prevState;
    int i;
    int first;

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
    //
    // Verify state machine state field
    //
    for (i = 0; i < MAX_STATES; i++) {
        // Assert state number is ordinal
        assert(SM[i].state == i);
        // Assert all states (except NONE) have state function
        assert(i == NONE || SM[i].func);
    }
    assert(NONE == 0);

    //
    // SM
    //
    prevState = NONE;
    state = OFF;
    first = 1;
    for (;;) {
        const struct TRANSITION * ptransition;
        CyBle_ProcessEvents();
        
        if (first) {
            xprintf("+State %s\r\n", SM[state].name);
        }
        SM[state].func(prevState, first ? FIRST_STATE_CALL : MIDDLE_STATE_CALL);
        first = 0;
        ptransition = &SM[state].transition[0];
        while (ptransition->newState != NONE) {
            if (!ptransition->condition || ptransition->condition(ptransition->condArg)) {
                int newState = ptransition->newState;
                xprintf("-State %s\r\n", SM[state].name);
                SM[state].func(newState, LAST_STATE_CALL);
                state = newState;
                first = 1;
                break;
            }
            ptransition++;
        }

        prevState = state;
    }
}
