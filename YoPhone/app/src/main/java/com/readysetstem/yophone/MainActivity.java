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

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.TextView;
import android.widget.Toast;

import java.util.UUID;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BleService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class MainActivity extends BleServiceConnectionActivity {
    private final static String TAG = MainActivity.class.getSimpleName();

    private final int REQUEST_CODE_BT_CONNECT = 1;

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    private TextView mTvConnectionState;
    private TextView mTvDeviceAddress;
    private TextView mTvDeviceName;
    private TextView mTvRxPackets;
    private TextView mTvRxBytes;
    private TextView mTvWatchState;
    private TextView mTvPrevState;
    private TextView mTvTransition;
    private TextView mTvVoiceCommand;
    private TextView mTvVoiceResult;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);
        Log.d(TAG, "onCreate()");

        // Sets up UI references.
        mTvDeviceAddress = (TextView) findViewById(R.id.device_address);
        mTvDeviceName = (TextView) findViewById(R.id.device_name);
        mTvConnectionState = (TextView) findViewById(R.id.connection_state);
        mTvRxBytes = (TextView) findViewById(R.id.rx_bytes);
        mTvRxPackets = (TextView) findViewById(R.id.rx_packets);
        mTvWatchState = (TextView) findViewById(R.id.watch_state);
        mTvPrevState = (TextView) findViewById(R.id.prev_state);
        mTvTransition = (TextView) findViewById(R.id.transition);
        mTvVoiceCommand = (TextView) findViewById(R.id.voice_command);
        mTvVoiceResult = (TextView) findViewById(R.id.voice_result);

        getSupportActionBar().setTitle(R.string.app_name);

        onBluetoothData(true);
    }

    protected void onResume()
    {
        super.onResume();
        onConnectionState();
    }

    protected void onPause()
    {
        super.onPause();

    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final Intent intent;
        switch (item.getItemId()) {
            case R.id.menu_connect:
                intent = new Intent(this, ConnectActivity.class);
                startActivityForResult(intent, REQUEST_CODE_BT_CONNECT);
                return true;
            case R.id.menu_settings:
                Toast.makeText(this, "settings", Toast.LENGTH_SHORT).show();
                return true;
            case R.id.menu_commands:
                Toast.makeText(this, "commands", Toast.LENGTH_SHORT).show();
                return true;
            case R.id.menu_log:
                Toast.makeText(this, "log", Toast.LENGTH_SHORT).show();
                return true;
            case R.id.menu_debug:
                intent = new Intent(this, DebugActivity.class);
                startActivity(intent);
                return true;
        }
        return super.onOptionsItemSelected(item);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent intent) {
        // DeviceListItem chose not to enable Bluetooth.
        Log.d(TAG, "onActivityResult");
        if (requestCode == REQUEST_CODE_BT_CONNECT && resultCode == Activity.RESULT_OK) {
            String deviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
            String deviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);
            mBleService.setDevice(deviceName, deviceAddress);
        }

        super.onActivityResult(requestCode, resultCode, intent);
    }

    public void onBluetoothData(final UUID characteristic, final byte[] data, final boolean clear) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final boolean show = mBleService != null && ! clear;

                mTvDeviceAddress.setText(show ? mBleService.getDeviceAddress() : "");
                mTvDeviceName.setText(show ? mBleService.getDeviceName() : "");
                mTvRxBytes.setText(show ? String.valueOf(mBleService.getRxBytes()) : "");
                mTvRxPackets.setText(show ? String.valueOf(mBleService.getRxPackets()) : "");
                mTvWatchState.setText(show ? mBleService.getWatchState() : R.string.nullstr);
                mTvPrevState.setText(show ? mBleService.getPrevState() : R.string.nullstr);
                mTvTransition.setText(show ? mBleService.getTransition() : R.string.nullstr);
                mTvVoiceCommand.setText(show ? mBleService.getVoiceCommand() : "");
                mTvVoiceResult.setText(show ? mBleService.getVoiceResult() : "");
            }
        });
    }

    public void onConnectionState() {
        super.onConnectionState();
        mTvConnectionState.setText(mConnectionStateRid);
    }
}