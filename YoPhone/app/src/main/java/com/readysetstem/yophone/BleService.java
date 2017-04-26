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

import android.app.Service;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCallback;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

import java.util.List;
import java.util.UUID;

/**
 * Service for managing connection and data communication with a GATT server hosted on a
 * given Bluetooth LE device.
 */
public class BleService extends Service {
    private final static String TAG = BleService.class.getSimpleName();
    public static final String PREFS_NAME = "com.readysetstem.yophone.prefs";

    public static final String EXTRAS_DEVICE_NAME = "DEVICE_NAME";
    public static final String EXTRAS_DEVICE_ADDRESS = "DEVICE_ADDRESS";

    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private String mBluetoothDeviceAddress;
    private BluetoothGatt mBluetoothGatt;

    private String mDeviceName;
    private String mDeviceAddress;
    private int mConnectionState = BluetoothProfile.STATE_DISCONNECTED;

    private SharedPreferences mSettings;

    private int mRxPackets = 0;

    private String[] characteristicsWithNotifications = {
            GattAttributes.CHARACTERISTIC_VOICE_DATA,
            GattAttributes.CHARACTERISTIC_DEBUG_COMMAND,
    };

    public final static String PKG = "com.readysetstem.yophone";
    public final static String ACTION_GATT_CONNECTION_CHANGE = PKG + ".ACTION_GATT_CONNECTION_CHANGE";
    public final static String ACTION_GATT_SERVICES_DISCOVERED =
            PKG + ".ACTION_GATT_SERVICES_DISCOVERED";
    public final static String ACTION_DATA_AVAILABLE = PKG + ".ACTION_DATA_AVAILABLE";
    public final static String EXTRA_DATA = PKG + ".EXTRA_DATA";
    public final static String EXTRA_CHARACTERISTIC = PKG + ".EXTRA_CHARACTERISTIC";

    // Implements callback methods for GATT events that the app cares about.  For example,
    // connection change and services discovered.
    private final BluetoothGattCallback mGattCallback = new BluetoothGattCallback() {
        @Override
        public void onConnectionStateChange(BluetoothGatt gatt, int status, int newState) {
            Log.i(TAG, "Dis/Connected to GATT server. (" + newState + ")");
            mConnectionState = newState;
            broadcastUpdate(ACTION_GATT_CONNECTION_CHANGE);

            if (newState == BluetoothProfile.STATE_CONNECTED) {
                // Attempts to discover services after successful connection.
                Log.i(TAG, "Attempting to start service discovery:" +
                        mBluetoothGatt.discoverServices());
            }
        }

        @Override
        public void onServicesDiscovered(BluetoothGatt gatt, int status) {
            Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");
            for (String c : characteristicsWithNotifications) {
                setCharacteristicNotification(
                        getCharacteristic(GattAttributes.SERVICE_SMARTWATCH, c), true);
            }
            exchangeGattMtu(512);
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_GATT_SERVICES_DISCOVERED);
            } else {
                Log.w(TAG, "onServicesDiscovered received: " + status);
            }
        }

        @Override
        public void onCharacteristicRead(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
        }

        @Override
        public void onCharacteristicWrite(BluetoothGatt gatt,
                                         BluetoothGattCharacteristic characteristic,
                                         int status) {
            Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");
/*
            if (status == BluetoothGatt.GATT_SUCCESS) {
                broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            }
*/
        }

        @Override
        public void onCharacteristicChanged(BluetoothGatt gatt,
                                            BluetoothGattCharacteristic characteristic) {
            Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");
            broadcastUpdate(ACTION_DATA_AVAILABLE, characteristic);
            mRxPackets++;
        }
    };

    private void broadcastUpdate(final String action) {
        final Intent intent = new Intent(action);
        sendBroadcast(intent);
    }

    private void broadcastUpdate(final String action,
                                 final BluetoothGattCharacteristic characteristic)
    {
        final Intent intent = new Intent(action);

        Log.i(TAG, "broadcastUpdate()");
        final byte[] data = characteristic.getValue();
        intent.putExtra(EXTRA_CHARACTERISTIC, characteristic.getUuid().toString());
        if (data != null && data.length > 0) {
            intent.putExtra(EXTRA_DATA, data);
        }
        sendBroadcast(intent);
    }

    public class LocalBinder extends Binder {
        BleService getService() {
            return BleService.this;
        }
    }
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }
    @Override
    public boolean onUnbind(Intent intent) {
        return super.onUnbind(intent);
    }
    private final IBinder mBinder = new LocalBinder();

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");
/*        if (isDisconnected()) {
            connect();
        }
  */
return START_STICKY;
    }

    @Override
    public void onCreate() {
        Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");

        // Restore preferences
        mSettings = getSharedPreferences(PREFS_NAME, 0);
        mDeviceAddress = mSettings.getString(EXTRAS_DEVICE_ADDRESS, null);
        mDeviceName = mSettings.getString(EXTRAS_DEVICE_NAME, null);
        Log.d(TAG, "Address: " + mDeviceAddress);


        // For API level 18 and above, get a reference to BluetoothAdapter through
        // BluetoothManager.
        if (mBluetoothManager == null) {
            mBluetoothManager = (BluetoothManager) getSystemService(Context.BLUETOOTH_SERVICE);
            if (mBluetoothManager == null) {
                Log.e(TAG, "Unable to initialize BluetoothManager.");
                return;
            }
        }

        mBluetoothAdapter = mBluetoothManager.getAdapter();
        if (mBluetoothAdapter == null) {
            Log.e(TAG, "Unable to obtain a BluetoothAdapter.");
            return;
        }
    }

    @Override
    public void onDestroy() {
        Log.d(TAG, "Entering: " + Thread.currentThread().getStackTrace()[2].getMethodName() + "()");
        disconnect();
        super.onDestroy();
    }

    /**
     * Connects to the GATT server hosted on the Bluetooth LE device.
     *
     * @return Return true if the connection is initiated successfully. The connection result
     *         is reported asynchronously through the
     *         {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     *         callback.
     */
    private boolean connect() {
        if (mBluetoothAdapter == null || mDeviceAddress == null) {
            Log.w(TAG, "Bluetooth not initialized or no address specified.");
            return false;
        }
        mConnectionState = BluetoothProfile.STATE_CONNECTING;
        broadcastUpdate(ACTION_GATT_CONNECTION_CHANGE);

        // Previously connected device.  Try to reconnect.
        if (mBluetoothGatt != null &&
                mDeviceAddress.equals(mBluetoothGatt.getDevice().getAddress()))
        {
            Log.d(TAG, "Trying to use an existing mBluetoothGatt for connection.");
            return mBluetoothGatt.connect();
        }

        final BluetoothDevice device;
        try {
            device = mBluetoothAdapter.getRemoteDevice(mDeviceAddress);
        } catch (IllegalArgumentException e) {
            Log.w(TAG, "Illegal bluetooth address: " + mDeviceAddress);
            return false;
        }
        if (device == null) {
            Log.w(TAG, "Device not found.  Unable to connect.");
            return false;
        }

        // We want to directly connect to the device, so we are setting the autoConnect
        // parameter to false.
        mBluetoothGatt = device.connectGatt(this, false, mGattCallback);
        Log.d(TAG, "Trying to create a new connection.");

        return true;
    }

    /**
     * Disconnects an existing connection or cancel a pending connection. The disconnection result
     * is reported asynchronously through the
     * {@code BluetoothGattCallback#onConnectionStateChange(android.bluetooth.BluetoothGatt, int, int)}
     * callback.
     */
    private void disconnect() {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "Bluetooth not initialized");
            return;
        }
        mConnectionState = BluetoothProfile.STATE_DISCONNECTING;
        broadcastUpdate(ACTION_GATT_CONNECTION_CHANGE);
        mBluetoothGatt.disconnect();
        mBluetoothGatt.close();
        mBluetoothGatt = null;
    }

    /**
     * Request a read on a given {@code BluetoothGattCharacteristic}. The read result is reported
     * asynchronously through the {@code BluetoothGattCallback#onCharacteristicRead(android.bluetooth.BluetoothGatt, android.bluetooth.BluetoothGattCharacteristic, int)}
     * callback.
     *
     * @param characteristic The characteristic to read from.
     */
    public void readCharacteristic(BluetoothGattCharacteristic characteristic) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.readCharacteristic(characteristic);
    }

    public void writeCharacteristic(BluetoothGattCharacteristic characteristic) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.writeCharacteristic(characteristic);
    }


    /**
     * Gets a characteristic given the service and characteristic attr strings
     *
     * @param serviceUuidString Service attr UUID string
     * @param characteristicUuidString Characteristic attr UUID string
     */
    public BluetoothGattCharacteristic getCharacteristic(String serviceUuidString,
                                                         String characteristicUuidString) {
        final UUID UUID_SERVICE = UUID.fromString(serviceUuidString);
        final UUID UUID_CHARACTERISTIC = UUID.fromString(characteristicUuidString);

        BluetoothGattService service = mBluetoothGatt.getService(UUID_SERVICE);
/*
        for (BluetoothGattCharacteristic i: service.getCharacteristics()) {
            Log.i(TAG, i.getUuid().toString());
        }
*/
        // TODO Get service may be null if not connected.
        return service.getCharacteristic(UUID_CHARACTERISTIC);
    }

    /**
     * Enables or disables notification on a give characteristic.
     *
     * @param characteristic Characteristic to act on.
     */
    public void setCharacteristicNotification(BluetoothGattCharacteristic characteristic,
                                              Boolean enabled) {
        if (mBluetoothAdapter == null || mBluetoothGatt == null) {
            Log.w(TAG, "BluetoothAdapter not initialized");
            return;
        }
        mBluetoothGatt.setCharacteristicNotification(characteristic, enabled);
    }

    /**
     * Retrieves a list of supported GATT services on the connected device. This should be
     * invoked only after {@code BluetoothGatt#discoverServices()} completes successfully.
     *
     * @return A {@code List} of supported services.
     */
    public List<BluetoothGattService> getSupportedGattServices() {
        if (mBluetoothGatt == null) return null;

        return mBluetoothGatt.getServices();
    }

    public int getConnectionState() {
        return mConnectionState;
/*        if (mBluetoothManager == null || mBluetoothGatt == null) {
            return BluetoothProfile.STATE_DISCONNECTED;
        }

        return mBluetoothManager.getConnectionState(
                mBluetoothGatt.getDevice(), BluetoothProfile.GATT);
*/    }

    public boolean isConnected() {
        return getConnectionState() == BluetoothProfile.STATE_CONNECTED;
    }
    public boolean isDisconnected() {
        return getConnectionState() == BluetoothProfile.STATE_DISCONNECTED;
    }

    public void exchangeGattMtu(int mtu) {
        int retry = 5;
        boolean status = false;
        while (!status && retry > 0) {
            status = mBluetoothGatt.requestMtu(mtu);
            retry--;
        }
    }

    public void setDevice(String name, String address) {
        mDeviceName = name;
        mDeviceAddress = address;

        SharedPreferences.Editor editor = mSettings.edit();
        editor.putString(EXTRAS_DEVICE_ADDRESS, mDeviceAddress);
        editor.putString(EXTRAS_DEVICE_NAME, mDeviceName);
        editor.commit();

        connect();
    }

    public String getDeviceName() {
        return mDeviceName;
    }
    public String getDeviceAddress() {
        return mDeviceAddress;
    }
    public int getRxBytes() {
        return 0;
    }
    public int getRxPackets() {
        return mRxPackets;
    }
    public int getWatchState() {
        return R.string.nullstr;
    }
    public int getPrevState() {
        return R.string.nullstr;
    }
    public int getTransition() {
        return R.string.nullstr;
    }
    public String getVoiceCommand() {
        return "";
    }
    public String getVoiceResult() {
        return "";
    }
}
