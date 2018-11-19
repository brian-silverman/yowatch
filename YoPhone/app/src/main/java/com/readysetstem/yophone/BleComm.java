/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.readysetstem.yophone;

import android.bluetooth.BluetoothGattCharacteristic;
import android.graphics.Color;
import android.os.Environment;
import android.support.v7.app.ActionBar;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;
import android.media.MediaPlayer;
import android.net.Uri;

import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.UUID;
import java.nio.charset.StandardCharsets;


/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BleService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class BleComm {
    private final static String TAG = BleComm.class.getSimpleName();
    private static final BleComm INSTANCE = new BleComm();

    private BleService mBleService;

    private BleComm() {}

    private int byteToInt(byte b) {
        return b < 0 ? 256 + b : b;
    }

    private int bytesToWord(byte high, byte low) {
        return (byteToInt(high) << 8) + byteToInt(low);
    }

    public void setService(BleService bleService) {
        mBleService = bleService;
    }

    public static BleComm getInstance() {
        return INSTANCE;
    }

    public void Send(byte[] data) {
        if (mBleService == null) return;
        BluetoothGattCharacteristic characteristic = mBleService.getCharacteristic(
                GattAttributes.SERVICE_SMARTWATCH, GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
        characteristic.setValue(data);
        mBleService.writeCharacteristic(characteristic);
    }

    public void Send(int v) {
        if (mBleService == null) return;
        BluetoothGattCharacteristic characteristic = mBleService.getCharacteristic(
                GattAttributes.SERVICE_SMARTWATCH, GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
        characteristic.setValue(v, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        mBleService.writeCharacteristic(characteristic);
    }

    public void Recv(BluetoothGattCharacteristic characteristic) {
        // TODO: header length is currently hardcoded
        byte[] data = characteristic.getValue();
        int word1 = bytesToWord(data[1], data[0]);
        int index = bytesToWord(data[3], data[2]);
        int payloadLen = bytesToWord(data[5], data[4]);
        int cmd = word1 & 0x1F;
        Log.d(TAG, "Recv: len: " + data.length + "(" + payloadLen + "), cmd: " + cmd);
        switch (cmd) {
            case BleApi.BLE_CMD_TEST_IDENTITY:
                break;
            case BleApi.BLE_CMD_TEST_INCREMENT:
                if (payloadLen > 150) {
                    int x = 0;
                }
                for (int i = 6; i < 6 + payloadLen; i++) {
                    data[i] += 1;
                }
                break;
            case BleApi.BLE_CMD_TEST_REVERSE:
                for (int i = 0; i < payloadLen/2; i++) {
                    byte tmp = data[6 + payloadLen - i - 1];
                    data[6 + payloadLen - i - 1] = data[6 + i];
                    data[6 + i] = tmp;
                }
                break;
            default:
                for (int i = 6; i < 6 + payloadLen; i++) {
                    data[i] = 0;
                }
                break;
        }
        Send(data);
    }
}

