/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef H_TEMP_
#define H_TEMP_

#ifdef __cplusplus
extern "C" {
#endif

static const ble_uuid16_t gatt_svr_chr_sec_test_envsvc_uuid =
        BLE_UUID16_INIT(0x181A);

static const ble_uuid16_t gatt_svr_chr_sec_test_envsens_uuid =
        BLE_UUID16_INIT(0x2A1E);

static const ble_uuid16_t gatt_svr_chr_sec_test_envsens2_uuid =
        BLE_UUID16_INIT(0x2A58);

static const ble_uuid16_t gatt_svr_chr_sec_test_esens_uuid =
        BLE_UUID16_INIT(0x290C);

static const ble_uuid128_t gatt_svr_svc_temp_uuid =
    BLE_UUID128_INIT(0x40, 0xb3, 0x20, 0x90, 0x72, 0xb5, 0x80, 0xaf,
                     0xa7, 0x4f, 0x15, 0x1c, 0xaf, 0xd2, 0x61, 0xe7);
                     
#define TEMP_TYPE 0x2A1E
#define TEMP_SNS_STRING "TEMPERATURE SENSOR"
#define TEMP_READ 0x2A3C

uint16_t gatt_temp_val[10];
uint16_t gatt_temp_val_len;
int temp_index;

int16_t get_temp_measurement(void);

#ifdef __cplusplus
}
#endif

#endif
