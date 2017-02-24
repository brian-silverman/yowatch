package com.readysetstem.yophone;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.widget.Toast;

public class MainActivity extends AppCompatActivity {
    private BluetoothAdapter mBluetoothAdapter;
    private Context mContext;
    private boolean mScanning = true;

    /**
     * Call back for BLE Scan
     * This call back is called when a BLE device is found near by.
     */
    private BluetoothAdapter.LeScanCallback mLeScanCallback = new BluetoothAdapter.LeScanCallback() {

        @Override
        public void onLeScan(final BluetoothDevice device, final int rssi,
                             byte[] scanRecord) {
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (mScanning) {
                        final Intent intent = new Intent(mContext, DeviceControlActivity.class);
                        intent.putExtra(DeviceControlActivity.EXTRAS_DEVICE_NAME, device.getName());
                        intent.putExtra(DeviceControlActivity.EXTRAS_DEVICE_ADDRESS, device.getAddress());
                        mBluetoothAdapter.stopLeScan(mLeScanCallback);
                        startActivity(intent);
                        mScanning = false;
                    }
                }
            });
        }
    };

    /**
     * Method to detect whether the device is phone or tablet
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        mContext = this;

        String x = DeviceControlActivity.EXTRAS_DEVICE_NAME;

        // Use this check to determine whether BLE is supported on the device.
        if (!this.getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_BLUETOOTH_LE)) {
            Toast.makeText(getApplicationContext(), R.string.device_ble_not_supported,
                    Toast.LENGTH_SHORT).show();
            finish();
        }
        // Initializes a Blue tooth adapter.
        final BluetoothManager bluetoothManager = (BluetoothManager)
                getSystemService(Context.BLUETOOTH_SERVICE);
        mBluetoothAdapter = bluetoothManager.getAdapter();

        if (mBluetoothAdapter == null) {
            // Device does not support Blue tooth
            Toast.makeText(getApplicationContext(),
                    R.string.device_bluetooth_not_supported, Toast.LENGTH_SHORT)
                    .show();
            finish();
        }

        mBluetoothAdapter.startLeScan(mLeScanCallback);
    }
}
