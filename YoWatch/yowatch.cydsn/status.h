#ifndef _STATUS_H_
#define _STATUS_H_

#define STATUS_BAR_HEIGHT   8   // 5x5 font plus bottom pixel border (3)
#define STATUS_BAR_WIDTH    SCREEN_WIDTH

//
// NOTIFications display left justified on the status bar, and only when the
// notificationsa have occured.  STATUSes appear right-justified, and generally
// stay there at all times.
//
enum NOTIF_CODES {
    FIRST_NOTIF = 0,
    NOTIF_PHONE = FIRST_NOTIF,
    NOTIF_MAIL,
    NOTIF_TEXT,
    NOTIF_ALERT,
    NOTIF_INFO,
    LAST_NOTIF
};

#define NUM_NOTIFS (LAST_NOTIF - FIRST_NOTIF)
#define IS_NOTIF(s) ((s) >= FIRST_NOTIF && (s) < LAST_NOTIF)

enum STATUS_CODES {
    FIRST_STATUS = NUM_NOTIFS,
    STATUS_BLE = FIRST_STATUS,
    STATUS_WIFI,
    STATUS_CELLULAR,
    STATUS_WATCH_BATTERY,
    STATUS_PHONE_BATTERY,
    STATUS_TIME,
    LAST_STATUS
};

#define NUM_STATUSES (LAST_STATUS - FIRST_STATUS)
#define IS_STATUS(s) ((s) >= FIRST_STATUS && (s) < LAST_STATUS)

enum STATUS_VALUES {
    STATUS_HIDE = 0,
    STATUS_SHOW = 1,
    STATUS_BLE_CONNECTED,
    STATUS_BLE_DISCONNECTED,
    STATUS_PERCENT_0,
    STATUS_PERCENT_25,
    STATUS_PERCENT_50,
    STATUS_PERCENT_75,
    STATUS_PERCENT_100,
};

int8 StatusGet(
    int code
    );
    
void StatusSet(
    int code,
    int8 value
    );

char StatusChar(
    int code,
    int8 val
    );

void StatusGetString(
    char * str,
    int firstCode,
    int lastCode
    );

#endif
