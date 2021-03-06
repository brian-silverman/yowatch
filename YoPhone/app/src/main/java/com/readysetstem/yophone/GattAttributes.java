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

import java.util.HashMap;

public class GattAttributes {
    private static HashMap<String, String> attributes = new HashMap();
    public static String SERVICE_SMARTWATCH = "C3113C46-D632-4380-9454-BFAB0F8E2871";
    public static String CHARACTERISTIC_VOICE_DATA = "93335079-B3B0-4ACD-97F7-F3A0DC528CFF";
    public static String CHARACTERISTIC_CONFIG = "CB0F1A66-24F1-4A4D-ABF6-4B789027E1C9";
    public static String CHARACTERISTIC_DEBUG_COMMAND = "67656849-90C0-47DF-8077-6D68F319BA46";

    static {
        attributes.put(SERVICE_SMARTWATCH, "Smartwatch Service");
        attributes.put(CHARACTERISTIC_VOICE_DATA, "Voice Data Characteristic");
        attributes.put(CHARACTERISTIC_CONFIG, "Config Characteristic");
        attributes.put(CHARACTERISTIC_DEBUG_COMMAND, "Debug Command Characteristic");
    }

    public static String lookup(String uuid, String defaultName) {
        String name = attributes.get(uuid);
        return name == null ? defaultName : name;
    }
}
