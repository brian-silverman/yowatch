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
import android.support.v7.app.ActionBar;
import android.os.Bundle;
import android.util.Log;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.TextView;

import java.util.UUID;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BleService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class DebugActivity extends BleServiceConnectionActivity {
    private final static String TAG = DebugActivity.class.getSimpleName();

    private ActionBar mActionBar;
    private TextView mTvSpeedtestKbps;
    private ImageView mIvSpeedtestPlay;
    private int mLen;
    private int mSpeedtestPackets;
    private long mStartTime;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.debug_activity);

        Log.d(TAG, "onCreate()");

        // Sets up UI references.
        mTvSpeedtestKbps = (TextView) findViewById(R.id.speedtest_kbps);
        mIvSpeedtestPlay = (ImageView) findViewById(R.id.speedtest_play);

        mActionBar = getSupportActionBar();
        mActionBar.setTitle(R.string.debug_title);
        mActionBar.setDisplayHomeAsUpEnabled(true);

        onConnectionState();
        onBluetoothData(true);
    }

    public void onBleServiceConnected() {
        Log.i(TAG, "onBleServiceConnected()");
        // TODO could simplify with a single write function
        BluetoothGattCharacteristic characteristic = mBleService.getCharacteristic(
                GattAttributes.SERVICE_SMARTWATCH, GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
        Log.i(TAG, characteristic.getUuid().toString());
        characteristic.setValue(1, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        mBleService.writeCharacteristic(characteristic);
    }

    public void onBluetoothData(final UUID characteristic, final byte[] data, final boolean clear) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final boolean show = (mBleService == null) ? false : !clear;

                // TODO convert characteristic passed around to UUIDs always (except accross intents)
                Log.i(TAG, "onBluetoothData: " + (characteristic != null ? characteristic : "null") + "," + GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
                if (characteristic != null && characteristic.equals(UUID.fromString(GattAttributes.CHARACTERISTIC_DEBUG_COMMAND))) {
                    Log.i(TAG, "onBluetoothData: " + (new Byte(data[0])).toString());
                    switch (data[0]) {
                        // TODO Need to make hardcoded command constants
                        case 3:
                            mSpeedtestPackets++;
                            mTvSpeedtestKbps.setText(show ? String.valueOf(mSpeedtestPackets) + "%" : "");
                            mLen = data.length;
                            break;
                        case 4:
                            final float time = System.currentTimeMillis() - mStartTime;
                            final float kbps = (100 * mLen * 8) / time;
                            mTvSpeedtestKbps.setText(String.format("%.1f kbps", kbps));
                            mIvSpeedtestPlay.setEnabled(true);
                            mIvSpeedtestPlay.setColorFilter(null);
                            break;
                    }
                }
            }
        });
    }

    public void onConnectionState() {
        super.onConnectionState();
        mActionBar.setTitle(getString(R.string.debug_title) + " (" + getString(mConnectionStateRid) + ")");
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch(item.getItemId()) {
            case android.R.id.home:
                onBackPressed();
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    public void onClickSpeedTest(View view) {
        // TODO Verify packets as they come in (incrementing uint32)
        BluetoothGattCharacteristic characteristic = mBleService.getCharacteristic(
                GattAttributes.SERVICE_SMARTWATCH, GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
        // TODO Do not use hardcoded constants
        characteristic.setValue(2, BluetoothGattCharacteristic.FORMAT_UINT8, 0);
        characteristic.setValue(100, BluetoothGattCharacteristic.FORMAT_UINT32, 1);
        mBleService.writeCharacteristic(characteristic);

        mSpeedtestPackets = 0;
        mStartTime = System.currentTimeMillis();
        mIvSpeedtestPlay.setEnabled(false);
        mIvSpeedtestPlay.setColorFilter(Color.argb(150,200,200,200));
    }
}