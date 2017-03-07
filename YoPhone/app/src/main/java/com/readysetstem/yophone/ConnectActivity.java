package com.readysetstem.yophone;

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


import android.support.v7.app.ActionBar;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.support.v7.app.AppCompatActivity;
import android.view.LayoutInflater;
import android.view.MenuItem;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;

/**
 * Activity for scanning and displaying available Bluetooth LE devices.
 */
public class ConnectActivity extends AppCompatActivity {
    private DeviceListAdapter mLeDeviceListAdapter;
    private BluetoothAdapter mBluetoothAdapter;
    private ListView mListView;
    private Context mContext;
    private ActionBar mActionBar;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.connect_activity);
        mContext = this;

        mListView = (ListView) findViewById(R.id.list_view);
        mListView.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            @Override
            public void onItemClick(AdapterView<?> parent, View view, int position, long id) {
                final BluetoothDevice device =
                        mLeDeviceListAdapter.getItem(position).mBluetoothDevice;
                if (device == null) return;
                mBluetoothAdapter.stopLeScan(mLeScanCallback);
                final Intent intent = new Intent(mContext, MainActivity.class);
                intent.putExtra(MainActivity.EXTRAS_DEVICE_NAME, device.getName());
                intent.putExtra(MainActivity.EXTRAS_DEVICE_ADDRESS, device.getAddress());
                setResult(RESULT_OK, intent);
                finish();
            }
        });

        mActionBar = getSupportActionBar();
        final Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            int dots = 0;
            @Override
            public void run() {
                mActionBar.setTitle(getString(R.string.connect_activity_title) + "....".substring(0, dots));
                dots = (dots + 1) % 4;
                handler.postDelayed(this, 500);
            }
        }, 0);

        mActionBar.setDisplayHomeAsUpEnabled(true);

        // Use this check to determine whether BLE is supported on the device.  Then you can
        // selectively disable BLE-related features.
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(this, R.string.ble_not_supported, Toast.LENGTH_SHORT).show();
            finish();
        }

        // Initializes a Bluetooth adapter.  For API level 18 and above, get a reference to
        // BluetoothAdapter through BluetoothManager.
        final BluetoothManager bluetoothManager =
                (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();

        // Checks if Bluetooth is supported on the device.
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, R.string.error_bluetooth_not_supported, Toast.LENGTH_SHORT).show();
            finish();
            return;
        }
    }

    @Override
    protected void onResume() {
        super.onResume();

        // Ensures Bluetooth is enabled on the device.  If Bluetooth is not currently enabled,
        // fire an intent to display a dialog asking the user to grant permission to enable it.
        if (!mBluetoothAdapter.isEnabled()) {
            Toast.makeText(this, R.string.bluetooth_not_enabled, Toast.LENGTH_SHORT).show();
        }

        mLeDeviceListAdapter = new DeviceListAdapter(this, new ArrayList<DeviceListItem>());
        mListView.setAdapter(mLeDeviceListAdapter);

        mBluetoothAdapter.startLeScan(mLeScanCallback);
    }

    @Override
    protected void onPause() {
        super.onPause();
        mBluetoothAdapter.stopLeScan(mLeScanCallback);
        mLeDeviceListAdapter.clear();
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

    private class DeviceListItem {
        public BluetoothDevice mBluetoothDevice;
        public int mRssi;
        public DeviceListItem(BluetoothDevice b, int rssi) {
            mBluetoothDevice = b;
            mRssi = rssi;
        }
    }

    public class DeviceListAdapter extends ArrayAdapter<DeviceListItem> {
        public DeviceListAdapter(Context context, ArrayList<DeviceListItem> users) {
            super(context, 0, users);
        }

        public void addDevice(BluetoothDevice device, int rssi) {
            for (int i = 0; i < getCount(); i++) {
                DeviceListItem item = getItem(i);
                if (item.mBluetoothDevice.getAddress().equals(device.getAddress())) {
                    item.mRssi = rssi;
                    return;
                }
            }
            add(new DeviceListItem(device, rssi));
        }

        @Override
        public View getView(int position, View convertView, ViewGroup parent) {
            // Get the data item for this position
            DeviceListItem user = getItem(position);

            // Check if an existing view is being reused, otherwise inflate the view
            if (convertView == null) {
                convertView = LayoutInflater.from(getContext()).inflate(R.layout.listitem_device, parent, false);
            }

            // Lookup view for data population
            TextView deviceAddress = (TextView) convertView.findViewById(R.id.device_address);
            TextView deviceName = (TextView) convertView.findViewById(R.id.device_name);
            TextView deviceRssi = (TextView) convertView.findViewById(R.id.device_rssi);

            // Populate the data into the template view using the data object
            deviceAddress.setText(user.mBluetoothDevice.getAddress());
            deviceName.setText(user.mBluetoothDevice.getName());
            deviceRssi.setText("RSSI: " + String.valueOf(-user.mRssi));

            // Return the completed view to render on screen
            return convertView;
        }
    }

    // Device scan callback.
    private BluetoothAdapter.LeScanCallback mLeScanCallback =
            new BluetoothAdapter.LeScanCallback() {

                @Override
                public void onLeScan(final BluetoothDevice device, final int rssi, byte[] scanRecord) {
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            mLeDeviceListAdapter.addDevice(device, rssi);
                            mLeDeviceListAdapter.notifyDataSetChanged();
                        }
                    });
                }
            };
}