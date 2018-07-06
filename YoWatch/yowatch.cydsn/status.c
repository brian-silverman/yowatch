/*
 * status.c
 *
 * Status bar functions
 *
 * Copyright (C) 2018 Brian Silverman <bri@readysetstem.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 */
#include <project.h>
#include "printf.h"
#include "assert.h"
#include "status.h"

#define CHR_PHONE               'P'
#define CHR_MAIL                'M'
#define CHR_TEXT                'T'
#define CHR_ALERT               'A'
#define CHR_INFO                'I'
#define CHR_BLE_CONNECTED       'B'
#define CHR_BLE_DISCONNECTED    'D'
#define CHR_WIFI_0              'a'
#define CHR_WIFI_20             'b'
#define CHR_WIFI_40             'c'
#define CHR_WIFI_60             'd'
#define CHR_WIFI_80             'e'
#define CHR_WIFI_100            'f'
#define CHR_CELLULAR_0          'j'
#define CHR_CELLULAR_20         'k'
#define CHR_CELLULAR_40         'l'
#define CHR_CELLULAR_60         'm'
#define CHR_CELLULAR_80         'n'
#define CHR_CELLULAR_100        'o'
#define CHR_BATTERY_0           'p'
#define CHR_BATTERY_20          'r'
#define CHR_BATTERY_40          's'
#define CHR_BATTERY_60          't'
#define CHR_BATTERY_80          'u'
#define CHR_BATTERY_100         'v'

static int8 statusCodes[NUM_STATUSES];
static int8 notifCodes[NUM_NOTIFS];

//
// Get the value of a status bar code.  May be called asynchronously.
//
int8 StatusGet(
    int code
    )
{
    assert(IS_NOTIF(code) || IS_STATUS(code));
    return IS_NOTIF(code) ? notifCodes[code] : statusCodes[code];
}
    
//
// Set the value of a status bar code.  May be called asynchronously.
//
void StatusSet(
    int code,
    int8 value
    )
{
    assert(IS_NOTIF(code) || IS_STATUS(code));
    if (IS_NOTIF(code)) {
        notifCodes[code] = value;
    } else {
        statusCodes[code] = value;
    }
}


char StatusChar(
    int code,
    int8 val
    )
{
    #define CV(c,v) (((c) << 8) | (v))
    switch (CV(code, val)) {
        case CV(NOTIF_PHONE, STATUS_SHOW):                  return CHR_PHONE;
        case CV(NOTIF_MAIL, STATUS_SHOW):                   return CHR_MAIL;
        case CV(NOTIF_TEXT, STATUS_SHOW):                   return CHR_TEXT;
        case CV(NOTIF_ALERT, STATUS_SHOW):                  return CHR_ALERT;
        case CV(NOTIF_INFO, STATUS_SHOW):                   return CHR_INFO;
        case CV(STATUS_BLE, STATUS_SHOW):                   return CHR_BLE_CONNECTED;
        case CV(STATUS_BLE, STATUS_BLE_CONNECTED):          return CHR_BLE_CONNECTED;
        case CV(STATUS_BLE, STATUS_BLE_DISCONNECTED):       return CHR_BLE_DISCONNECTED;
        case CV(STATUS_WIFI, STATUS_SHOW):                  return CHR_WIFI_0;
        case CV(STATUS_WIFI, STATUS_PERCENT_0):             return CHR_WIFI_0;
        case CV(STATUS_WIFI, STATUS_PERCENT_20):            return CHR_WIFI_20;
        case CV(STATUS_WIFI, STATUS_PERCENT_40):            return CHR_WIFI_40;
        case CV(STATUS_WIFI, STATUS_PERCENT_60):            return CHR_WIFI_60;
        case CV(STATUS_WIFI, STATUS_PERCENT_80):            return CHR_WIFI_80;
        case CV(STATUS_WIFI, STATUS_PERCENT_100):           return CHR_WIFI_100;
        case CV(STATUS_CELLULAR, STATUS_SHOW):              return CHR_CELLULAR_0;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_0):         return CHR_CELLULAR_0;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_20):        return CHR_CELLULAR_20;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_40):        return CHR_CELLULAR_40;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_60):        return CHR_CELLULAR_60;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_80):        return CHR_CELLULAR_80;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_100):       return CHR_CELLULAR_100;
        case CV(STATUS_WATCH_BATTERY, STATUS_SHOW):         return CHR_BATTERY_0;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_0):    return CHR_BATTERY_0;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_20):   return CHR_BATTERY_20;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_40):   return CHR_BATTERY_40;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_60):   return CHR_BATTERY_60;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_80):   return CHR_BATTERY_80;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_100):  return CHR_BATTERY_100;
        case CV(STATUS_PHONE_BATTERY, STATUS_SHOW):         return CHR_BATTERY_0;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_0):    return CHR_BATTERY_0;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_20):   return CHR_BATTERY_20;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_40):   return CHR_BATTERY_40;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_60):   return CHR_BATTERY_60;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_80):   return CHR_BATTERY_80;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_100):  return CHR_BATTERY_100;
        default: return '\0';
    }
}

void StatusGetString(
    char * str,
    int firstCode,
    int lastCode
    )
{
    int c = 0;
    for (int code = firstCode; code < lastCode; code++) {
        int8 val = StatusGet(code);
        if (val) {
            str[c++] = StatusChar(code, val);
        }
    }
}
