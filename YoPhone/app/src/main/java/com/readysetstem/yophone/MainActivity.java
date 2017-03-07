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
import android.bluetooth.BluetoothGattCharacteristic;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.widget.ExpandableListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

/**
 * For a given BLE device, this Activity provides the user interface to connect, display data,
 * and display GATT services and characteristics supported by the device.  The Activity
 * communicates with {@code BluetoothLeService}, which in turn interacts with the
 * Bluetooth LE API.
 */
public class MainActivity extends AppCompatActivity {
    private final static String TAG = MainActivity.class.getSimpleName();
    public static final String PREFS_NAME = "com.readysetstem.yophone.prefs";

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

    private int mRxPackets = 0;
    private int mRxBytes = 0;
    private String mDeviceName;
    private String mDeviceAddress;
    private BluetoothLeService mBluetoothLeService;
    private SharedPreferences mSettings;

    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBluetoothLeService = ((BluetoothLeService.LocalBinder) service).getService();
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBluetoothLeService = null;
        }
    };

    // Handles various events fired by the Service.
    // ACTION_GATT_CONNECTED: connected to a GATT server.
    // ACTION_GATT_DISCONNECTED: disconnected from a GATT server.
    // ACTION_GATT_SERVICES_DISCOVERED: discovered GATT services.
    // ACTION_DATA_AVAILABLE: received data from the device.  This can be a result of read
    //                        or notification operations.
    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            if (BluetoothLeService.ACTION_DATA_AVAILABLE.equals(action)) {
                updateConnectionState(true);
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main_activity);

        Log.d(TAG, "onCreate()");

        // Restore preferences
        mSettings = getSharedPreferences(PREFS_NAME, 0);
        mDeviceAddress = mSettings.getString(EXTRAS_DEVICE_ADDRESS, null);
        mDeviceName = mSettings.getString(EXTRAS_DEVICE_NAME, null);
        Log.d(TAG, "Address: " + mDeviceAddress);

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

        updateConnectionState(false);

        getSupportActionBar().setTitle(getString(R.string.app_name));

        Intent intent = new Intent(this, BluetoothLeService.class);
        intent.putExtra(EXTRAS_DEVICE_NAME, mDeviceName);
        intent.putExtra(EXTRAS_DEVICE_ADDRESS, mDeviceAddress);
        startService(intent);
        bindService(intent, mServiceConnection, 0);

    }

    @Override
    protected void onResume() {
        super.onResume();
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(BluetoothLeService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BluetoothLeService.ACTION_DATA_AVAILABLE);
        registerReceiver(mGattUpdateReceiver, intentFilter);
    }

    @Override
    protected void onPause() {
        super.onPause();
        unregisterReceiver(mGattUpdateReceiver);
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG, "onDestroy()");
        super.onDestroy();
        unbindService(mServiceConnection);
        mBluetoothLeService = null;
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.main_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        final Intent intent;
        switch(item.getItemId()) {
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
            mDeviceName = intent.getStringExtra(EXTRAS_DEVICE_NAME);
            mDeviceAddress = intent.getStringExtra(EXTRAS_DEVICE_ADDRESS);

            SharedPreferences.Editor editor = mSettings.edit();
            editor.putString(EXTRAS_DEVICE_ADDRESS, mDeviceAddress);
            editor.putString(EXTRAS_DEVICE_NAME, mDeviceName);
            editor.commit();
        }

        super.onActivityResult(requestCode, resultCode, intent);
    }

    private void updateConnectionState(final boolean display) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                boolean d = display;
                if (mBluetoothLeService == null) {
                    d = false;
                }
                mTvConnectionState.setText(d ? mBluetoothLeService.getConnectionState() : "");
                mTvDeviceAddress.setText(d ? mBluetoothLeService.getDeviceAddress() : "");
                mTvDeviceName.setText(d ? mBluetoothLeService.getDeviceName() : "");
                mTvRxBytes.setText(d ? String.valueOf(mBluetoothLeService.getRxBytes()) : "");
                mTvRxPackets.setText(d ? String.valueOf(mBluetoothLeService.getRxPackets()) : "");
                mTvWatchState.setText(d ? mBluetoothLeService.getWatchState() : R.string.nullstr);
                mTvPrevState.setText(d ? mBluetoothLeService.getPrevState() : R.string.nullstr);
                mTvTransition.setText(d ? mBluetoothLeService.getTransition() : R.string.nullstr);
                mTvVoiceCommand.setText(d ? mBluetoothLeService.getVoiceCommand() : "");
                mTvVoiceResult.setText(d ? mBluetoothLeService.getVoiceResult() : "");
            }
        });
    }
}
