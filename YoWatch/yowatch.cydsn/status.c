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
#define CHR_WIFI_25             'b'
#define CHR_WIFI_50             'c'
#define CHR_WIFI_75             'd'
#define CHR_WIFI_100            'e'
#define CHR_CELLULAR_0          'h'
#define CHR_CELLULAR_25         'i'
#define CHR_CELLULAR_50         'j'
#define CHR_CELLULAR_75         'k'
#define CHR_CELLULAR_100        'l'
#define CHR_BATTERY_0           'p'
#define CHR_BATTERY_25          'q'
#define CHR_BATTERY_50          'r'
#define CHR_BATTERY_75          's'
#define CHR_BATTERY_100         't'
#define CHR_INVALID             'Z'

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
    return IS_NOTIF(code) ? notifCodes[code - FIRST_NOTIF] : statusCodes[code - FIRST_STATUS];
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
        notifCodes[code - FIRST_NOTIF] = value;
    } else {
        statusCodes[code - FIRST_STATUS] = value;
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
        case CV(STATUS_WIFI, STATUS_PERCENT_25):            return CHR_WIFI_25;
        case CV(STATUS_WIFI, STATUS_PERCENT_50):            return CHR_WIFI_50;
        case CV(STATUS_WIFI, STATUS_PERCENT_75):            return CHR_WIFI_75;
        case CV(STATUS_WIFI, STATUS_PERCENT_100):           return CHR_WIFI_100;
        case CV(STATUS_CELLULAR, STATUS_SHOW):              return CHR_CELLULAR_0;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_0):         return CHR_CELLULAR_0;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_25):        return CHR_CELLULAR_25;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_50):        return CHR_CELLULAR_50;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_75):        return CHR_CELLULAR_75;
        case CV(STATUS_CELLULAR, STATUS_PERCENT_100):       return CHR_CELLULAR_100;
        case CV(STATUS_WATCH_BATTERY, STATUS_SHOW):         return CHR_BATTERY_0;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_0):    return CHR_BATTERY_0;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_25):   return CHR_BATTERY_25;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_50):   return CHR_BATTERY_50;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_75):   return CHR_BATTERY_75;
        case CV(STATUS_WATCH_BATTERY, STATUS_PERCENT_100):  return CHR_BATTERY_100;
        case CV(STATUS_PHONE_BATTERY, STATUS_SHOW):         return CHR_BATTERY_0;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_0):    return CHR_BATTERY_0;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_25):   return CHR_BATTERY_25;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_50):   return CHR_BATTERY_50;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_75):   return CHR_BATTERY_75;
        case CV(STATUS_PHONE_BATTERY, STATUS_PERCENT_100):  return CHR_BATTERY_100;
        default: return CHR_INVALID;
    }
}

void StatusGetString(
    char * str,
    int firstCode,
    int lastCode
    )
{
    int c = 0;
    str[0] = 0;
    for (int code = firstCode; code < lastCode; code++) {
        int8 val = StatusGet(code);
        if (val) {
            str[c++] = StatusChar(code, val);
        }
    }
    str[c] = 0;
}
