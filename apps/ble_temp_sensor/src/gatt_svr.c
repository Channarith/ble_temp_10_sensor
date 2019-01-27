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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <host/ble_hs.h>
#include <host/ble_uuid.h>
#include "bsp/bsp.h"

#include "ble_temp_sensor.h"
#include "temp.h"
//#include "bleprph.h"


/* 5c3a659e-897e-45e1-b016-007107c96df7 */
static const ble_uuid128_t gatt_svr_chr_temp_uuid =
        BLE_UUID128_INIT(0xf7, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);

/* 5c3a659e-897e-45e1-b016-007107c96dff */
static const ble_uuid128_t gatt_svr_chr_sec_test_rand_uuid =
        BLE_UUID128_INIT(0xff, 0x6d, 0xc9, 0x07, 0x71, 0x00, 0x16, 0xb0,
                         0xe1, 0x45, 0x7e, 0x89, 0x9e, 0x65, 0x3a, 0x5c);


/* 5c3a659e-897e-45e1-b016-007107c96dff */


// This is the service for reading the BLOB...
static int
gatt_svr_sns_access(uint16_t conn_handle, uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt,
    void *arg);

    
static int
gatt_svr_chr_cb(uint16_t conn_handle,
                uint16_t attr_handle,
                struct ble_gatt_access_ctxt *ctxt, void *arg);


static int
gatt_svr_chr_rand_cb(uint16_t conn_handle,
                uint16_t attr_handle,
                struct ble_gatt_access_ctxt *ctxt, void *arg);


static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;
	LOG(INFO, "============HELLO\n");
    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}


static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Temperature Sensor */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        //.uuid = &gatt_svr_svc_temp_uuid.u,
        .uuid = &gatt_svr_chr_sec_test_envsvc_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[]) { {
            /* Characteristic: Temperature measurement */
            .flags = BLE_GATT_CHR_F_READ,
            .uuid = &gatt_svr_chr_sec_test_envsens_uuid.u,
            .access_cb = gatt_svr_chr_cb,
        	}, 
        	// RANDOM NUMBER GENERATOR
        	{
        		.flags = BLE_GATT_CHR_F_READ,
        		.uuid = &gatt_svr_chr_sec_test_envsens2_uuid.u,
        		.access_cb = gatt_svr_chr_rand_cb,        
        		//.descriptors.uuid = &gatt_svr_chr_sec_test_envsens_uuid.u,
        		.descriptors = (struct ble_gatt_dsc_def[]) { { 
        			.att_flags = BLE_GATT_CHR_F_READ,
        			.uuid = &gatt_svr_chr_sec_test_esens_uuid.u,
        			.access_cb = gatt_svr_chr_rand_cb,        
        		},
				}
        	}, {
        		0, // No more characteristics in this service
        	} },        	
      },
          {
            /*** TEMPERATURE Level Notification Service. */
            .type = BLE_GATT_SVC_TYPE_PRIMARY,
            .uuid = &gatt_svr_svc_temp_uuid.u,
            .characteristics = (struct ble_gatt_chr_def[]) { {
                .uuid = BLE_UUID16_DECLARE(TEMP_TYPE),
                .access_cb = gatt_svr_sns_access,
                .flags = BLE_GATT_CHR_F_READ,
            }, {
                .uuid = BLE_UUID16_DECLARE(TEMP_READ),
                .access_cb = gatt_svr_sns_access,
                .flags = BLE_GATT_CHR_F_NOTIFY,
            }, {
                0, /* No more characteristics in this service. */
            } },
        },               	
    {
        0, /* No more services */
    },    
};

static int
gatt_svr_sns_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt,
                          void *arg)
{
    uint16_t uuid16;
    int rc;

    uuid16 = ble_uuid_u16(ctxt->chr->uuid);

    switch (uuid16) {
    case TEMP_TYPE:
        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
        int16_t temp; 

	    //temp = get_temp_measurement();

        rc = os_mbuf_append(ctxt->om, &gatt_temp_val, sizeof(gatt_temp_val));
        int i=0;
        LOG(INFO, "\n\n LAST 10 TEMPERATUREs\n");
        for (i=0;i<10;i++) {
        	LOG(INFO," 0x%x,", gatt_temp_val[i]);
        }
        LOG(INFO, "\n\n");
        
        //LOG(INFO, "GATT VAL: %i\n", gatt_temp_val);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

    case TEMP_READ:
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            LOG(INFO, "TEMP ACCESS: %i\n", gatt_temp_val);
            
            rc = gatt_svr_chr_write(ctxt->om, 0,
                                    sizeof(gatt_temp_val),
                                    &gatt_temp_val,
                                    &gatt_temp_val_len);                                    
            return rc;
        } else if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
        	LOG(INFO, "TEMP ACCESS READ: %i\n", gatt_temp_val);

			rc = os_mbuf_append(ctxt->om, &gatt_temp_val, sizeof(temp));

            //rc = os_mbuf_append(ctxt->om, &gatt_temp_val, sizeof(gatt_temp_val));
            return rc; // == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        } else {
        	LOG(INFO, "\n\n\n->      OP: %i\n\n\n",ctxt->op);
        	
        }

    default:
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }
}

static int
gatt_svr_chr_cb(
    uint16_t conn_handle,
    uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    int16_t temp;
    
    

    temp = get_temp_measurement();
    
    LOG(INFO, "read value=%i\n", temp);	
    rc = os_mbuf_append(ctxt->om, &temp, sizeof(temp));
    	
    return rc;
}


static int
gatt_svr_chr_rand_cb(
    uint16_t conn_handle,
    uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    int rc;
    int temp;
    
    
    //# ===================================
    //# CHAN ADDED
    //# Random 16 bit number generator
    //# ===================================	
    //temp = (rand() & 0xFFFF);
    temp = rand();
    //temp = get_prev10temp_measurement();

	int swapped = ((temp>>24)&0xff) | // move byte 3 to byte 0
                    ((temp<<8)&0xff0000) | // move byte 1 to byte 2
                    ((temp>>8)&0xff00) | // move byte 2 to byte 1
                    ((temp<<24)&0xff000000); // byte 0 to byte 3
    
    //struct ble_gatt_chr_def *chr_def;
 	//struct ble_gatt_dsc_def *dsc_def;
 	//chr_def->descriptors = &gatt_svr_chr_sec_test_envsens_uuid.u;
 	//dsc_def->uuid = &gatt_svr_chr_sec_test_envsens_uuid.u;
 	   
    //ctxt->chr->descriptors = &gatt_svr_chr_sec_test_envsens_uuid.u;                
    //ctxt->dsc->uuid = &gatt_svr_chr_sec_test_envsens_uuid.u;                

    LOG(INFO, "RANDOM NUMBER=0x%x\n", swapped);	
    rc = os_mbuf_append(ctxt->om, &temp, sizeof(temp));
    	
    return rc;
}


void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        LOG(DEBUG, "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        LOG(DEBUG, "registered characteristic %s with "
                           "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        LOG(DEBUG, "registered descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
gatt_svr_init(void)
{
    int rc;

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

