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
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <nrf_temp.h>

#include <os/mynewt.h>
#include <os/os.h>
#include <nimble/ble.h>
#include <host/ble_hs.h>
#include <services/gap/ble_svc_gap.h>
#include <bsp/bsp.h>
#include <hal/hal_gpio.h>
#include <sysinit/sysinit.h>

#include "ble_temp_sensor.h"
#include "temp.h"
#include "gatt_svr.h"

/* Log data */
struct log logger;

static const char *device_name = "ble_temp_sensor";

static int ble_temp_gap_event(struct ble_gap_event *event, void *arg);

static uint8_t ble_temp_addr_type;

// CHAN ADDED
#define TEMPERATURE_PRIO 128
#define TEMPERATURE_STACK_SIZE (OS_STACK_ALIGN(512))
struct os_eventq temp_evq;
struct os_task temp_task;
bssnz_t os_stack_t temp_stack[TEMPERATURE_STACK_SIZE];

/*
#define GEN_TASK_PRIO       3     
#define GEN_TASK_STACK_SZ   512
#define TASK_LED        17

static os_stack_t gen_task_stack[GEN_TASK_STACK_SZ];
// Task for temperature reading
static struct os_task gen_task_str;

int g_led_pin;
*/

/* Callback function for application task event */
//static void my_ev_cb(struct os_event *);

/* Initialize the event with the callback function */
//static struct os_event gen_task_ev = {
//    .ev_cb = my_ev_cb,
//};

//static void my_ev_cb(struct os_event *ev)
//{
    //assert(ev);
    //hal_gpio_toggle(TASK_LED);
//    return;
//}


/*
 * Enables advertising with parameters:
 *     o General discoverable mode
 *     o Undirected connectable mode
 */
static void
ble_temp_advertise(void)
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    int rc;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                    BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        LOG(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(ble_temp_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_temp_gap_event, NULL);
    if (rc != 0) {
        LOG(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }
}


static int
ble_temp_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed */
        LOG(INFO, "connection %s; status=%d\n",
                    event->connect.status == 0 ? "established" : "failed",
                    event->connect.status);

        if (event->connect.status != 0) {
            /* Connection failed; resume advertising */
            ble_temp_advertise();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        LOG(INFO, "disconnect; reason=%d\n", event->disconnect.reason);

        /* Connection terminated; resume advertising */
        ble_temp_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        LOG(INFO, "adv complete\n");
        ble_temp_advertise();
        break;


    case BLE_GAP_EVENT_MTU:
        LOG(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                    event->mtu.conn_handle,
                    event->mtu.value);
        break;

    }

    return 0;
}

static void
on_sync(void)
{
    int rc;

    /* Use privacy */
    rc = ble_hs_id_infer_auto(0, &ble_temp_addr_type);
    assert(rc == 0);

    /* Begin advertising */
    ble_temp_advertise();

    LOG(INFO, "adv started\n");
}


/*
// # CHAN ADDED
static void
gen_temp_task(void *arg)
{

  	//struct os_task *t;

	//g_led_pin = LED_BLINK_PIN;
    //hal_gpio_init_out(g_led_pin, 1);
    
    // CIRCULAR BUFFER
    // Decided to not use pointers for head and tail. We'll just keep track of the index.
	// This way we don't need to write traversal routines and max checks
 	int temp_index=0;
    int16_t temp[10]={0};

    //# ===================================
    //# CHAN ADDED
    //# Loop of 10 temperature readings
    //# ===================================
    while (1) {
    	
    	// Start the temperature measurement. 
    	NRF_TEMP->TASKS_START = 1;
    	while(NRF_TEMP->EVENTS_DATARDY != TEMP_INTENSET_DATARDY_Set) {};
    	// Temp reading is in units of 0.25degC, so divide by 4 to get in units of degC
     	// * (scale by 100 to avoid representing as decimal). 
    	temp[temp_index] = (nrf_temp_read() * 100) / 4.0;

    	LOG(INFO, "---> [%i, %i, %i, %i, %i, %i, %i, %i, %i, %i]\n", temp[0],temp[1],temp[2],temp[3],temp[4],temp[5],temp[6],temp[7],temp[8],temp[9] );	
    	
    	//rc = os_mbuf_append(ctxt->om, &temp, sizeof(temp));
    	
    	//# Delay of 100ms
    	os_cputime_delay_usecs(100000);
    	//os_eventq_put(os_eventq_dflt_get(), &gen_task_ev);
    	
    	// Increment to the next index of the circular buffer
    	temp_index=(temp_index + 1) % 10;
	}
	
}
*/

/*
static void
init_temp_readings(void)
{
	os_stack_t *work;
	work = malloc(sizeof(os_stack_t)*GEN_TASK_STACK_SZ);
	assert(work);
	
    // Create a task to generate events to toggle the LED at pin TASK_LED 
    os_task_init(&gen_task_str, "gen_temp_task", gen_temp_task, NULL, GEN_TASK_PRIO,
                 OS_WAIT_FOREVER, gen_task_stack, GEN_TASK_STACK_SZ);

    

}
*/

int temp_read_event(void)
{
    // CIRCULAR BUFFER
    // Decided to not use pointers for head and tail. We'll just keep track of the index.
	// This way we don't need to write traversal routines and max checks
 	//int temp_index=0;
    //int16_t temp[10]={};
    uint16_t chr_val_handle;
	//uint16_t chr_val_handle[10]={0};
	int rc;
	//int a=0;
    //# ===================================
    //# CHAN ADDED
    //# ===================================
    //for (a=0; a<10;a++) {
    	
    	// Start the temperature measurement. 
    	NRF_TEMP->TASKS_START = 1;
    	while(NRF_TEMP->EVENTS_DATARDY != TEMP_INTENSET_DATARDY_Set) {};
    	// Temp reading is in units of 0.25degC, so divide by 4 to get in units of degC
     	// * (scale by 100 to avoid representing as decimal). 
    	gatt_temp_val[temp_index] = (nrf_temp_read() * 100) / 4.0;

    	LOG(INFO, "---> [%i, %i, %i, %i, %i, %i, %i, %i, %i, %i]\n", gatt_temp_val[0],gatt_temp_val[1],gatt_temp_val[2],gatt_temp_val[3],gatt_temp_val[4],gatt_temp_val[5],gatt_temp_val[6],gatt_temp_val[7],gatt_temp_val[8],gatt_temp_val[9] );	

		//gatt_temp_val[temp_index] = temp[temp_index];	
		temp_index=(temp_index + 1) % 10;

	
    	//# Delay of 100ms
    	// os_cputime_delay_usecs(100000);
    	// os_eventq_put(os_eventq_dflt_get(), &gen_task_ev);
    	// no longer needed
    	
    	// Increment to the next index of the circular buffer
	
	//}
	//gatt_temp_val = temp;
	gatt_temp_val_len = sizeof(gatt_temp_val);
    rc = ble_gatts_find_chr(&gatt_svr_svc_temp_uuid.u, BLE_UUID16_DECLARE(TEMP_TYPE), NULL, &chr_val_handle);
	//assert(rc == 0);
	
	ble_gatts_chr_updated(chr_val_handle);

	
	return rc;

}

static void
temp_task_handler(void *unused)
{
	while (1) {
		temp_read_event();
		
		// DELAY of 100ms
		os_time_delay(OS_TICKS_PER_SEC * 0.1);
	}
}

/*
 * main
 *
 * The main task for the project. This function initializes the packages,
 * then starts serving events from default event queue.
 *
 * @return int NOTE: this function should never return!
 */
int
main(void)
{
    int rc;
	temp_index=0;
	
    /* Initialize OS */
    sysinit();

    /* Initialize the logger */
    log_register("ble_temp_sensor_log", &logger, &log_console_handler, NULL, LOG_SYSLEVEL);

    /* Prepare the internal temperature module for measurement */
    nrf_temp_init();

    /* Prepare BLE host and GATT server */
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.gatts_register_cb = gatt_svr_register_cb;

    rc = gatt_svr_init();
    assert(rc == 0);

    /* Set the default device name */
    rc = ble_svc_gap_device_name_set(device_name);
    assert(rc == 0);
    
    // # Initialize event q for pumping out temperature readings every 100ms...
    // For debugging, you can see on the serial console the circular buffer measurements
    // This is asynchronous.
    //init_temp_readings();

	os_eventq_init(&temp_evq);
	os_task_init(&temp_task,"Temperature Blob", temp_task_handler, NULL, TEMPERATURE_PRIO, OS_WAIT_FOREVER, temp_stack, TEMPERATURE_STACK_SIZE);
	

    /* As the last thing, process events from default event queue */
    while (1) {
        os_eventq_run(os_eventq_dflt_get());
    }
    return 0;
}

