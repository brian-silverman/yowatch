/*
 * debug_locally.c
 *
 * Manual debugging interface on the YoWatch hardware (does not require BLE).
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

unsigned char cmds[][2] = {
    {0xFD,0},
    {0x12,1},
    {0xFD,0},
    {0xB1,1},
    {0xAE,0},
    {0xB3,0},
    {0xF1,0},
    {0xCA,0},
    {0x7F,1},
    {0xA0,0},
    {0x74,1},
    {0x15,0},
    {0x00,1},
    {0x7F,1},
    {0x75,0},
    {0x00,1},
    {0x7F,1},
    {0xA1,0},
    {0x60,1},
    {0xA2,0},
    {0x00,1},
    {0xB5,0},
    {0x00,1},
    {0xAB,0},
    {0x01,1},
    {0xB1,0},
    {0x32,0},
    {0xBE,0},
    {0x05,0},
    {0xA6,0},
    {0xC1,0},
    {0xC8,1},
    {0x80,1},
    {0xC8,1},
    {0xC7,0},
    {0x0F,1},
    {0xB4,0},
    {0xA0,1},
    {0xB5,1},
    {0x55,1},
    {0xB6,0},
    {0x01,1},
    {0xAF,0},
};

unsigned char square[][2] = {
    {0x15,0},
    {0x32,1},
    {0x36,1},
    {0x75,0},
    {0x14,1},
    {0x18,1},
    {0x5C,0},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
    {0xF8,1},
    {0x00,1},
};

//
// Debug Locally State Machine
//
// @return 1 if done with SM, else 0
//
int DebugLocallySM(
    )
{
    static int i = 0;
    int j;
    char s[32];
    if (i % 100000 == 0) {
        sprintf(s, "%d\r\n", i);
        UART_1_PutString(s);
    }
    i++;
    if (i % 1000000 == 200000) {
        UART_1_PutString("Configure...\r\n");
        SPI_1_SpiSetActiveSlaveSelect(SPI_1_SPI_SLAVE_SELECT2);
        for (j = 0; j < sizeof(cmds)/sizeof(cmds[0]); j++) {
            OLED_DAT_CMD_Write(cmds[j][1]);
            SPI_1_SpiUartWriteTxData(cmds[j][0]);
            CyDelayUs(5);
        }
        UART_1_PutString("Write square...\r\n");
        for (j = 0; j < sizeof(square)/sizeof(square[0]); j++) {
            OLED_DAT_CMD_Write(square[j][1]);
            SPI_1_SpiUartWriteTxData(square[j][0]);
            CyDelayUs(5);
        }
        UART_1_PutString("done...\r\n");
    }
    return 0;
}
