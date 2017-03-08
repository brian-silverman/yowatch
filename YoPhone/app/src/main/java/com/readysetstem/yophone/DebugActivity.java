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

import android.support.v7.app.ActionBar;
import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BleService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class DebugActivity extends BleServiceConnectionActivity {
    private final static String TAG = DebugActivity.class.getSimpleName();

    private TextView mTvRxPackets;
    private ActionBar mActionBar;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.debug_activity);

        Log.d(TAG, "onCreate()");

        // Sets up UI references.
        mTvRxPackets = (TextView) findViewById(R.id.rx_packets);

        mActionBar = getSupportActionBar();
        mActionBar.setTitle(R.string.debug_title);
        mActionBar.setDisplayHomeAsUpEnabled(true);

        updateConnectionState();
        updateBluetoothData(true);
    }

    public void updateBluetoothData(final boolean clear) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final boolean show = (mBleService == null) ? false : ! clear;

                mTvRxPackets.setText(show ? String.valueOf(mBleService.getRxPackets()) : "");
            }
        });
    }

    public void updateConnectionState() {
        super.updateConnectionState();
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

}