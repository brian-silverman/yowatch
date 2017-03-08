package com.readysetstem.yophone;

import android.bluetooth.BluetoothProfile;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;

public class BleServiceConnectionActivity extends AppCompatActivity {
    private final static String TAG = BleServiceConnectionActivity.class.getSimpleName();
    public BleService mBleService;
    public int mConnectionStateRid = R.string.connection_state_disconnected;

    // Code to manage Service lifecycle.
    private final ServiceConnection mServiceConnection = new ServiceConnection() {
        @Override
        public void onServiceConnected(ComponentName componentName, IBinder service) {
            mBleService = ((BleService.LocalBinder) service).getService();
        }

        @Override
        public void onServiceDisconnected(ComponentName componentName) {
            mBleService = null;
        }
    };

    // Handles various events fired by the Service.
    // ACTION_GATT_CONNECTION_CHANGE: connected to a GATT server.
    // ACTION_GATT_SERVICES_DISCOVERED: discovered GATT services.
    // ACTION_DATA_AVAILABLE: received data from the device.  This can be a result of read
    //                        or notification operations.
    private final BroadcastReceiver mGattUpdateReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            final String action = intent.getAction();
            updateConnectionState();
            if (BleService.ACTION_GATT_CONNECTION_CHANGE.equals(action)) {
                updateBluetoothData(true);
            } else if (BleService.ACTION_DATA_AVAILABLE.equals(action)) {
                updateBluetoothData();
            }
        }
    };

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = new Intent(this, BleService.class);
        startService(intent);
        bindService(intent, mServiceConnection, 0);
    }

    @Override
    protected void onResume() {
        super.onResume();
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(BleService.ACTION_GATT_CONNECTION_CHANGE);
        intentFilter.addAction(BleService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(BleService.ACTION_DATA_AVAILABLE);
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
        mBleService = null;
    }

    public void updateConnectionState() {
        if (mBleService != null) {
            final int state = mBleService.getConnectionState();
            switch (state) {
                case BluetoothProfile.STATE_CONNECTED:
                    updateBluetoothData();
                    mConnectionStateRid = R.string.connection_state_connected;
                    break;
                case BluetoothProfile.STATE_DISCONNECTED:
                    mConnectionStateRid = R.string.connection_state_disconnected;
                    break;
                case BluetoothProfile.STATE_CONNECTING:
                    mConnectionStateRid = R.string.connection_state_connecting;
                    break;
                case BluetoothProfile.STATE_DISCONNECTING:
                    mConnectionStateRid = R.string.connection_state_disconnecting;
                    break;
                default:
                    mConnectionStateRid = R.string.wtf;
                    break;
            }
        } else {
            mConnectionStateRid = R.string.connection_state_disconnected;
        }
    }

    public void updateBluetoothData(boolean clear) {
    };

    public void updateBluetoothData() {
        updateBluetoothData(false);
    };
}