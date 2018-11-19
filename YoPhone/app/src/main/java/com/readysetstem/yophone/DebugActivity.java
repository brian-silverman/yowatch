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
public class DebugActivity extends BleServiceConnectionActivity {
    private final static String TAG = DebugActivity.class.getSimpleName();

    private ActionBar mActionBar;
    private TextView mTvSpeedtestKbps;
    private ImageView mIvSpeedtestPlay;
    private ImageView mIvMicPlay;
    private ImageView mIvMicStop;
    private ImageView mIvMicRecord;
    private ByteArrayOutputStream mMicRecording;
    private int mLen;
    private int mSpeedtestPackets;
    private int mmm;
    private long mStartTime;
    private String mMicFilename;

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.debug_activity);

        Log.d(TAG, "onCreate()");

        // Sets up UI references.
        mTvSpeedtestKbps = (TextView) findViewById(R.id.speedtest_kbps);
        mIvSpeedtestPlay = (ImageView) findViewById(R.id.speedtest_play);
        mIvMicPlay = (ImageView) findViewById(R.id.mic_play);
        mIvMicStop = (ImageView) findViewById(R.id.mic_stop);
        mIvMicRecord = (ImageView) findViewById(R.id.mic_record);

        mActionBar = getSupportActionBar();
        mActionBar.setTitle(R.string.debug_title);
        mActionBar.setDisplayHomeAsUpEnabled(true);

        mMicRecording = null;
        mMicFilename = Environment.getExternalStorageDirectory().getAbsolutePath() + "/micout.wav";

        onConnectionState();
        onBluetoothData(true);
    }

    public void onBleServiceConnected() {
        Log.i(TAG, "onBleServiceConnected()");
    }

    public void onBluetoothData(final UUID characteristic, final byte[] data, final boolean clear) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                final boolean show = (mBleService == null) ? false : !clear;

                UUID uuid = UUID.fromString(GattAttributes.CHARACTERISTIC_DEBUG_COMMAND);
                if (characteristic != null && characteristic.equals(uuid)) {
                    Log.i(TAG, "onBluetoothData: " + (new Byte(data[0])).toString());
                    switch (data[0]) {
                        case BleApi.BLE_W2P_SPEEDTEST_PACKET:
                            // TODO Verify packets as they come in (incrementing uint32)
                            mSpeedtestPackets++;
                            mTvSpeedtestKbps.setText(
                                show ? String.valueOf(mSpeedtestPackets) + "%" : ""
                                );
                            mLen = data.length;
                            break;
                        case BleApi.BLE_W2P_SPEEDTEST_FINAL:
                            final float time = System.currentTimeMillis() - mStartTime;
                            final float kbps = (100 * mLen * 8) / time;
                            mTvSpeedtestKbps.setText(String.format("%.1f kbps", kbps));
                            mIvSpeedtestPlay.setEnabled(true);
                            mIvSpeedtestPlay.setColorFilter(null);
                            break;
                        case BleApi.BLE_W2P_MIC_RECORDING:
                            if (mSpeedtestPackets % 32 == 0) {
                                mBleComm.Send(mmm);
                            }
                            mSpeedtestPackets++;
                            if (mMicRecording != null) {
                                try {
                                    mMicRecording.write(Arrays.copyOfRange(data, 4, data.length));
                                } catch (IOException e) {
                                    e.printStackTrace();
                                }
                            }
                            break;
                    }
                }
            }
        });
    }

    public void onConnectionState() {
        super.onConnectionState();
        mActionBar.setTitle(
            getString(R.string.debug_title) + " (" + getString(mConnectionStateRid) + ")"
            );
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
        mBleComm.Send(BleApi.BLE_P2W_SPEEDTEST_START);
        mSpeedtestPackets = 0;
        mStartTime = System.currentTimeMillis();
        mIvSpeedtestPlay.setEnabled(false);
        mIvSpeedtestPlay.setColorFilter(Color.argb(150,200,200,200));
    }

    public void onClickMicRecord(View view) {
        mBleComm.Send(BleApi.BLE_P2W_MIC_RECORD_START);

        mSpeedtestPackets = 0;
        mmm = BleApi.BLE_P2W_MIC_RECORD_CONTINUE;
        mMicRecording = new ByteArrayOutputStream();
        mIvMicRecord.setEnabled(false);
        mIvMicRecord.setColorFilter(Color.argb(150,200,200,200));
    }

    private byte[] DoubleWordToByteArray(long w) {
        return new byte[] {(byte) ((w>>0)&0xFF), (byte) ((w>>8)&0xFF),
                           (byte) ((w>>16)&0xFF), (byte) ((w>>24)&0xFF)};

    }
    private byte[] WordToByteArray(long w) {
        return new byte[] {(byte) ((w>>0)&0xFF), (byte) ((w>>8)&0xFF)};

    }

    public void onClickMicStop(View view) {
        if (mBleService == null) return;
        if (mMicRecording != null) {
            Log.i(TAG, "onClickMicStop()");

            //
            // WAV file header:
            //
            // 1) "RIFF"
            // 2) raw file size + 36 (0x000292c0 + 0x24 = 0x000292e4)
            // 3) "WAVE"
            // 4) "fmt "
            // 5) 16 (0x10) for PCM
            // 6a) 2 bytes: AudioFormat: PCM = 1
            // 6b) 2 bytes: Mono = 1
            // 7) Sample rate: 16kHz = 0x3e80
            // 8) ByteRate = SampleRate * NumChannels * BitsPerSample/8 = 32kHz = 0x7d00
            // 9a) 2 bytes: BlockAlign = NumChannels * BitsPerSample/8 = 2
            // 9b) 2 bytes: BitsPerSamples = 16
            // 10) "data"
            // 11) raw file size (0x000292c0)
            //
            try (OutputStream outputStream = new FileOutputStream(mMicFilename)) {
                outputStream.write("RIFF".getBytes(StandardCharsets.UTF_8));
                outputStream.write(DoubleWordToByteArray(mMicRecording.size() + 36));
                outputStream.write("WAVE".getBytes(StandardCharsets.UTF_8));
                outputStream.write("fmt ".getBytes(StandardCharsets.UTF_8));
                outputStream.write(DoubleWordToByteArray(0x10));
                outputStream.write(WordToByteArray(1));
                outputStream.write(WordToByteArray(1));
                outputStream.write(DoubleWordToByteArray(0x3e80));
                outputStream.write(DoubleWordToByteArray(0x7d00));
                outputStream.write(WordToByteArray(2));
                outputStream.write(WordToByteArray(16));
                outputStream.write("data".getBytes(StandardCharsets.UTF_8));
                outputStream.write(DoubleWordToByteArray(mMicRecording.size()));

                mMicRecording.writeTo(outputStream);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        mMicRecording = null;

        mmm = BleApi.BLE_P2W_MIC_RECORD_END;

        mIvMicRecord.setEnabled(true);
        mIvMicRecord.setColorFilter(null);
    }

    public void onClickMicPlay(View view) {
        Uri uri = Uri.parse("file://" + mMicFilename);
        MediaPlayer mp = MediaPlayer.create(this, uri);
        mp.start();
    }
        
}
