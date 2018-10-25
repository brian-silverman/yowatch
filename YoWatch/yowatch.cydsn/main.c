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
#include "fonts.h"
#include "colors.h"
#include "draw.h"
#include "status.h"
#include "util.h"
#include "tick.h"
#include "listen.h"

#define MIC_CHAR 'X'

//
// BLE buffer
//
static uint8 bleBuf[MAX_MTU_SIZE+4];

//
// Global commmunication flags between asynchornous BLE messages and state
// machine.  (Note: BLE async messages are actually synchornized with the state
// machine via the CyBle_ProcessEvents() call, so require no locking).
//
static int speedTestPackets;
static uint bleTransitionFlags = 0;
static int deviceConnected = 0;

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
#define BLE_SPEECH2TEXT     (1 << 8)
#define BLE_RESPONSE        (1 << 9)

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

#define SPECIAL_SPEED_TEST_END  (1 << 0)

//
// BLE receive packet callback
//
void OnBleRecv(
    uint8 * data,
    int len
    )
{
    // TODO Fix hardcoded constants
    switch (*data) {
        case 0: // Exit debug mode
            bleTransitionFlags = BLE_END_TEST;
            xprintf("Debug 0\r\n");
            break;
        case 1: // Go to debug
            xprintf("Debug 1\r\n");
            break;
        case 2: // Go to debug speedtest
            bleTransitionFlags = BLE_SPEED_TEST;
            speedTestPackets = 100; // TODO need correct # packets
            xprintf("Debug 2, %d\r\n", len);
            break;
        case 3: // debug mic
            bleTransitionFlags = BLE_MIC_TEST;
            xprintf("Debug 3\r\n");
            break;
        case 6:
            bleTransitionFlags = BLE_RESPONSE;
            //xprintf("Debug 6\r\n");
            break;
        default:
            xprintf("Debug ?\r\n");
            break;
    }
}

//
// BLE callback when connection changed
// TODO handle state change (in main state machine) when device disconnects
//
void OnConnectionChange(
    int connected
    )
{
    deviceConnected = connected;
}

//////////////////////////////////////////////////////////////////////
//
// State transition functions
//

int TrAccel(
    int state,
    int transitionType
    )
{
    return 0;
}

int TrBle(
    int state,
    int transitionType
    )
{
    uint doTransition = bleTransitionFlags & transitionType;
#if 0
    if (doTransition || bleTransitionFlags) {
        xprintf("%08X, %08X, %08X\r\n", doTransition, bleTransitionFlags, transitionType);
    }
#endif

    return doTransition;
}

int TrButton(
    int state,
    int transitionType
    )
{
    return 0;
}

int TrGoToSleep(
    int state,
    int transitionType
    )
{
    return 0;
}

int TrVoice(
    int state,
    int transitionType
    )
{
    return (transitionType == VOICE_ACTIVITY && ListenActivity())
        || (transitionType == VOICE_TIMEOUT && ListenTimeout());
}

int TrSpecial(
    int state,
    int transitionType
    )
{
    switch (transitionType) {
        case SPECIAL_SPEED_TEST_END:    return speedTestPackets < 0;    break;
    }

    return 0;
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
    // Note: SmOff only gets called (once) on power up...

    //
    // Initialization that's required at boot once SM is running.
    //
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
    static uint since1 = 0;
    static uint since2 = 0;
    int font = FONT_5X8;
    int micHeight = GetFontHeight(font);
    int micWidth = GetCharWidth(MIC_CHAR, font);

    DrawStatusBar(call == FIRST_STATE_CALL);

    if (call == FIRST_STATE_CALL) {
        ListenStart();

    } else if (call == MIDDLE_STATE_CALL) {
        // FIXME: currently doind seconds calc - switch to minutes
        if (Expired(1000, &since1)) {
            char s[10];
            uint curMinTick = GetTick() / 1000;
            sprintf(s, "%2d:%02d", curMinTick / 60, curMinTick % 60);
            char * a[] = {s};
            RECT r = (RECT) {
                0,
                STATUS_BAR_HEIGHT,
                SCREEN_WIDTH,
                SCREEN_HEIGHT - STATUS_BAR_HEIGHT - micHeight
                };
            DrawTextBox(a, 1, r, 0, HCENTER_JUSTIFIED | VCENTER_JUSTIFIED, FONT_TIME, WHITE, BLACK);

        }
        if (Expired(50, &since2)) {
            char s[] = {MIC_CHAR, 0};
            DrawText(s, 0, SCREEN_HEIGHT - micHeight, font, WHITE, BLACK);
            int level = ListenLevel();
            int barX = micWidth + 4;
            int barWidth = barX + (level * SCREEN_WIDTH / (2 * LISTEN_HIGH_THRESHOLD));
            int maxBarWidth = SCREEN_WIDTH - barX;

            RECT r;
            r = (RECT) {barX, SCREEN_HEIGHT - micHeight, barWidth, micHeight};
            DrawRect(r, GREEN);
            r = (RECT) 
                {barX + barWidth, SCREEN_HEIGHT - micHeight, maxBarWidth - barWidth, micHeight};
            DrawRect(r, BLACK);
        }

    } else if (call == LAST_STATE_CALL) {
        ListenStop();
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

int BleSendWait(
    uint8 * data,
    int len,
    int waitForResponse
    )
{
    static int waitingForResponse = 0;
    int ret = -EAGAIN;

    if (waitingForResponse) {
        if (bleTransitionFlags & BLE_RESPONSE) {
            waitingForResponse = 0;
        }
    } else {
        ret = BleSend(data, len);
        if (ret == 0 && waitForResponse) {
            waitingForResponse = 1;
        }
    }

    return ret;
}

void SmSpeedTest(
    int prevState,
    int call
    )
{
    static int n = 0;
    int ret;

    if (call == FIRST_STATE_CALL) {
        n = 0;
    } else if (call == MIDDLE_STATE_CALL) {
        if (CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
            if (speedTestPackets > 0) {
                ((uint32 *) bleBuf)[0] = 0;
                bleBuf[0] = 3;
                ((uint32 *) bleBuf)[1] = n;
                ((uint32 *) bleBuf)[2] = 0x11223344;

                ret = BleSendWait(bleBuf, 500, n % 32 == 0);
                if (ret == 0) {
                    speedTestPackets--;
                    n++;
                }
            } else if (speedTestPackets == 0) {
                ((uint32 *) bleBuf)[0] = 0;
                bleBuf[0] = 4;

                ret = BleSend(bleBuf, 1);
                if (ret == 0) {
                    speedTestPackets--;
                }
            }
        }
    } else if (call == LAST_STATE_CALL) {
    }
}

void SmMicTest(
    int prevState,
    int call
    )
{
    static int lastWasSent;
    static int n = 0;
    int ret;

    if (call == FIRST_STATE_CALL) {
        BufQueueInit();
        I2sStartDma(NULL, NULL);
        lastWasSent = 1;
        n = 0;

    } else if (call == MIDDLE_STATE_CALL) {
        if (CyBle_GattGetBusyStatus() == CYBLE_STACK_STATE_FREE) {
            if (lastWasSent) {
                ret = DequeueBytesBlocking(&bleBuf[4], 496);
                if (ret == -ENODATA) return;
            }

            ((uint32 *) bleBuf)[0] = 0;
            bleBuf[0] = 5;
            ret = BleSendWait(bleBuf, 500, n % 32 == 0);
            lastWasSent = ret == 0;
            if (lastWasSent) {
                n++;
            }
        }

    } else if (call == LAST_STATE_CALL) {
        I2sStopDma();
    }
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
#define MAX_STATE_TRANSITIONS 6
const struct {
    int state;
    char * name;
    void (*func)(int, int);
    struct TRANSITION {
        int (*transTest)(int, int);
        int transArg;
        int newState;
    } transition[MAX_STATE_TRANSITIONS];
} SM[MAX_STATES] = {
    { NAME(NONE) },
    { NAME(OFF), SmOff, {
        { NULL, 0, TIME }
        }},
    { NAME(SLEEP), SmSleep, {
        { TrAccel, ACCEL_TWIST, TIME },
        { TrButton, BUTTON_ANY, TIME },
        }},
    { NAME(TIME), SmTime, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        //{ TrVoice, VOICE_ACTIVITY, VOICE },
        { TrButton, BUTTON_FORWARD, MSGS },
        { TrBle, BLE_SPEED_TEST, SPEED_TEST }, //DEBUG (move to SLEEP)
        { TrBle, BLE_MIC_TEST, MIC_TEST }, //DEBUG (move to SLEEP)
        }},
    { NAME(VOICE), SmVoice, {
        { TrBle, BLE_DISCONNECT, DISCONNECT },
        { TrGoToSleep, 0, SLEEP },
        { TrVoice, VOICE_TIMEOUT, RUN_CMD },
        { TrButton, BUTTON_BACK, TIME },
        { TrBle, BLE_SPEECH2TEXT, VOICE },
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
        { TrBle, BLE_END_TEST, TIME /*SLEEP*/ },
        { TrButton, BUTTON_BACK, TIME },
        { TrSpecial, SPECIAL_SPEED_TEST_END, TIME /*SLEEP*/ },
        }},
    { NAME(MIC_TEST), SmMicTest, {
        { TrBle, BLE_END_TEST, TIME /*SLEEP*/ },
        { TrButton, BUTTON_BACK, TIME },
        }},
    { NAME(DISCONNECT), SmDisconnect, {
        { TrBle, BLE_CONNECT, TIME },
        }},
};

const struct {
    int (*transTest)(int, int);
    char * name;
} transitionNames[] = {
    { NAME(TrAccel) },
    { NAME(TrBle) },
    { NAME(TrButton) },
    { NAME(TrGoToSleep) },
    { NAME(TrVoice) },
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

    I2S_1_Start();
    Timer_1_Start();
    SPI_1_Start();
    SpiTxIsr_StartEx(SpiTxIsr);
    UART_1_Start();

    Timer_Programmable_Init();

    DisplayInit();
    SerialRamInit();
    BufQueueInit();
    Post();

    // Tick API timer may be used in Post(), so init after
    TickInit();

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
    uint since = 0;
    prevState = NONE;
    state = OFF;
    first = 1;
    for (;;) {
        const struct TRANSITION * ptransition;
        bleTransitionFlags = 0;
        CyBle_ProcessEvents();

        if (first) {
            xprintf("+State %s\r\n", SM[state].name);
        }
        if (Expired(1000, &since)) {
            xprintf("Tick %d, %d\r\n", GetTick(), CyBle_GetState());
        }

        SM[state].func(prevState, first ? FIRST_STATE_CALL : MIDDLE_STATE_CALL);
        first = 0;
        int transitionIndex = 0;
        ptransition = &SM[state].transition[transitionIndex];
        while (transitionIndex < MAX_STATE_TRANSITIONS && ptransition->newState != NONE) {
            if (!ptransition->transTest || ptransition->transTest(state, ptransition->transArg)) {
                char * trName;
                for (i = 0; i < ARRAY_SIZEOF(transitionNames); i++) {
                    if (ptransition->transTest == transitionNames[i].transTest) break;
                }
                trName = i >= ARRAY_SIZEOF(transitionNames) ? "???" : transitionNames[i].name;

                xprintf("-State %s (--- %s ---> %s)\r\n",
                    SM[state].name, trName, SM[ptransition->newState].name
                    );
                SM[state].func(ptransition->newState, LAST_STATE_CALL);
                state = ptransition->newState;
                first = 1;
                break;
            }
            ptransition++;
            transitionIndex++;
        }

        prevState = state;
    }
}
