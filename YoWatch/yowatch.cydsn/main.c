#include <main.h>
#include <BLEApplications.h>

uint8 buffer[MAX_MTU_SIZE];
int deviceConnected = 0;
int speedTestPackets;

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
    DEBUG_IDLE,
    DEBUG_SPEEDTEST,
    DEBUG_SPEEDTEST_2,
    DEBUG_VOICE,
} state, bleSelectedNextState;

void PrintState(
    enum STATE state
    )
{
    #define PRINT_CASE(state) case state: UART_UartPutString("State: " #state "\r\n"); break

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
        PRINT_CASE(DEBUG_IDLE);
        PRINT_CASE(DEBUG_SPEEDTEST);
        PRINT_CASE(DEBUG_SPEEDTEST_2);
        PRINT_CASE(DEBUG_VOICE);
    }
}

CYBLE_API_RESULT_T OnDebugCommandCharacteristic(
    CYBLE_GATT_DB_ATTR_HANDLE_T handle,
    uint8 * data, 
    int len
    ) 
{
    char s[32];
    sprintf(s, "%d, %d, %d\r\n", *data, len, state);
    UART_UartPutString(s);
    
    switch (*data) {
        case 0: // Exit debug mode
            bleSelectedNextState = SLEEP;
            break;
        case 1: // Go to debug
            if (state == SLEEP) {
                bleSelectedNextState = DEBUG_IDLE;
            }
            UART_UartPutString("Go to DEBUG_IDLE\r\n");
            PrintState(state);
            break;
        case 2: // Go to debug speedtest
            speedTestPackets = 100; // TODO need correct # packets
            if (state == DEBUG_IDLE) {
                bleSelectedNextState = DEBUG_SPEEDTEST;
            }
            break;
        default:
            bleSelectedNextState = NONE;
            break;
    }

    return 0;
}

void OnConnectionChange(
    int connected
    )
{
    uint8 c = 0;
    BleWriteCharacteristic(CYBLE_SMARTWATCH_SERVICE_VOICE_DATA_CHAR_HANDLE, &c, sizeof(c));
    deviceConnected = connected;
}

int main()
{
    uint32 n = 0;
    int ret;
    enum STATE newState, prevState;
    enum STATE pState;

    prevState = OFF;
    state = SLEEP;

    pState = OFF;
    
    CyGlobalIntEnable; 

    BleStart();    
    BleRegisterWriteCallback(
        OnDebugCommandCharacteristic,
        CYBLE_SMARTWATCH_SERVICE_SERVICE_INDEX, 
        CYBLE_SMARTWATCH_SERVICE_DEBUG_COMMAND_CHAR_INDEX
        );

    UART_Start();
    
    UART_UartPutString("--- STARTUP ---\r\n");

    int i = 0;
    for (;;) {
        CyBle_ProcessEvents();

        if (state != prevState) PrintState(state);
        if (bleSelectedNextState != pState) {
            UART_UartPutString("BLE: ");
            PrintState(bleSelectedNextState);
            pState = bleSelectedNextState;
        }

        newState = NONE;
        switch (state) {
            case OFF:
                newState = SLEEP;
                break;

            case SLEEP:
                if (bleSelectedNextState != NONE) {
                    newState = bleSelectedNextState;
                }
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
                if (bleSelectedNextState != NONE) {
                    newState = bleSelectedNextState;
                }
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
                break;

            case DEBUG_VOICE:
                break;
            
            default:
                break;
        }

        bleSelectedNextState = NONE;
        prevState = state;
        if (newState != NONE) {
            state = newState;
        }
    }
}
