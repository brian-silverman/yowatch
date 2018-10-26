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

    public static void Send(BleService bleService, int v) {
        if (bleService == null) return;
        BluetoothGattCharacteristic characteristic = bleService.getCharacteristic(
                GattAttributes.SERVICE_SMARTWATCH, GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
        characteristic.setValue(v, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        bleService.writeCharacteristic(characteristic);
    }

    public static void Recv(BleService bleService, BluetoothGattCharacteristic characteristic) {
        final byte[] data = characteristic.getValue();
        Log.d(TAG, "Recv: " + data.length);
        Send(bleService, 7);

    }
}

