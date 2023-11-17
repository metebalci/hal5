/*
 * SPDX-FileCopyrightText: 2023 Mete Balci
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (c) 2023 Mete Balci
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "hal5_usb.h"
#include "hal5_usb_device.h"

#define TRX_WINDEX_AS_ENDPOINT_NUMBER(trx) \
    ((uint8_t) (trx->device_request->wIndex & 0x000F))

#define TRX_WINDEX_AS_ENDPOINT_DIR_IN(trx) \
    ((bool) (trx->device_request->wIndex & 0x0080))

#define TRX_WINDEX_AS_INTERFACE_NUMBER(trx) \
    ((uint8_t) (trx->device_request->wIndex & 0x00FF))

// temporary storage of device_address
// stored at SETUP of SET ADDRESS
// used at IN_0 of SET ADDRESS
static uint8_t device_address = 0;

typedef enum
{
    standard_request_null,
    standard_request_device_get_status,
    standard_request_device_clear_feature,
    standard_request_device_set_feature,
    standard_request_device_set_address,
    standard_request_device_get_descriptor,
    standard_request_device_set_descriptor,
    standard_request_device_get_configuration,
    standard_request_device_set_configuration,
    standard_request_interface_get_status,
    standard_request_interface_clear_feature,
    standard_request_interface_set_feature,
    standard_request_interface_get_interface,
    standard_request_interface_set_interface,
    standard_request_endpoint_get_status,
    standard_request_endpoint_clear_feature,
    standard_request_endpoint_set_feature,
    standard_request_endpoint_synch_frame,
} usb_standard_request_t;

// used to temporarily store the standard request
// stored at SETUP, NULLed at setup_transaction_finished
static usb_standard_request_t standard_request = standard_request_null;

// SETUP TRANSACTION HELPERS

static void setup_transaction_finished(
        hal5_usb_transaction_t *trx)
{
    device_address = 0;
    standard_request = standard_request_null;
}

// SETUP IN_DATA OUT_0 e.g. get_descriptor
// this is used to send data for IN
static void setup_transaction_reply_in(
        hal5_usb_transaction_t *trx,
        const void* data, 
        const size_t len)
{
    memcpy(trx->tx_data, data, len);
    trx->tx_data_len = len;

    // IN = tx valid
    hal5_usb_prepare_endpoint(
            0, 
            ep_status_disabled,
            ep_status_valid);
}

// SETUP_IN_DATA OUT_0 e.g. get_descriptor
// this is used to acknowledge the OUT with zero data
static void setup_transaction_ack_out_zero(
        hal5_usb_transaction_t *trx)
{
    // OUT = rx valid
    hal5_usb_prepare_endpoint(
            0,
            ep_status_valid,
            ep_status_disabled);

    setup_transaction_finished(trx);
}

// SETUP IN_0 e.g. clear_feature
// SETUP OUT_DATA IN_0 e.g. set_descriptor

// this is used to confirm the request by sending a zero size data for in 
static void setup_transaction_reply_in_with_zero(
        hal5_usb_transaction_t *trx)
{
    trx->tx_data_len = 0;

    // IN = tx valid
    hal5_usb_prepare_endpoint(
            0, 
            ep_status_disabled,
            ep_status_valid);

    setup_transaction_finished(trx);
}

// stall IN as RequestError
static void setup_transaction_stall_in(
        hal5_usb_transaction_t *trx)
{
    // IN = tx stall
    hal5_usb_prepare_endpoint(
            0, 
            ep_status_disabled,
            ep_status_stall);

    setup_transaction_finished(trx);
}

// stall OUT as RequestError
static void setup_transaction_stall_out(
        hal5_usb_transaction_t *trx)
{
    // OUT = rx stall
    hal5_usb_prepare_endpoint(
            0, 
            ep_status_stall,
            ep_status_disabled);

    setup_transaction_finished(trx);
}

// STANDARD REQUESTS

static void device_get_status(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_get_status;

    assert (trx->device_request->wValue == 0);
    assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
        case usb_device_state_address: 
            break;
        default: assert (false);
    }

    uint8_t status[2] = {0};

    if (hal5_usb_device_is_device_self_powered_ex()) 
    {
        status[1] |= (1 << 0);
    }

    if (hal5_usb_device_is_device_remote_wakeup_set_ex()) 
    {
        status[1] |= (1 << 1);
    }

    setup_transaction_reply_in(trx, status, 2);
}

static void device_clear_feature(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_clear_feature;

    assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
        case usb_device_state_address: 
            break;
        default: assert (false);
    }

    const uint16_t feature_selector = 
        trx->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP) 
    {
        success = hal5_usb_device_clear_device_remote_wakeup_ex();
    }
    else if (feature_selector == FEATURE_SELECTOR_TEST_MODE)
    {
        // test mode cannot be cleared by Clear Feature
        setup_transaction_stall_in(trx);
    }

    if (success) setup_transaction_reply_in_with_zero(trx);
    else setup_transaction_stall_in(trx);
}

static void device_set_feature(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_set_feature;

    assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
        case usb_device_state_address: 
            break;
        default: assert (false);
    }

    const uint16_t feature_selector = 
        trx->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP) 
    {
        success = hal5_usb_device_set_device_remote_wakeup_ex();
    }
    else if (feature_selector == FEATURE_SELECTOR_TEST_MODE)
    {
        success = hal5_usb_device_set_test_mode_ex();
    }

    if (success) setup_transaction_reply_in_with_zero(trx);
    else setup_transaction_stall_in(trx);
}

static void device_set_address(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_set_address;

    assert (trx->device_request->wValue <= 127);
    assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 0);

    // it is possible that this request 
    // comes again in address state
    // or comes with address=0
    // see hal5_usb_device_set_address method
    // for more information

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_default: 
        case usb_device_state_address: 
            break;

        default: assert (false);
    }

    device_address = trx->device_request->wValue;

    printf("SET_ADDRESS: %u\n", device_address);

    // IMPORTANT: address is actually set after IN zero
    // this is different than all other requests

    setup_transaction_reply_in_with_zero(trx);
}

static void device_get_descriptor(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_get_descriptor;

    const uint32_t descriptor_type = 
        (trx->device_request->wValue >> 8) & 0xFF;
    
    switch (descriptor_type)
    {
        case 0x01:
            {
                // device descriptor (device should have one, must)
                printf("GET_DESCRIPTOR(device)\n");
                hal5_usb_device_descriptor_t descriptor;
                hal5_usb_device_get_device_descriptor_ex(&descriptor);
                setup_transaction_reply_in(
                        trx,
                        &descriptor,
                        sizeof(descriptor));
            }
            break;

        case 0x02:
            {
                // configuration descriptor (device should have at least one)
                // including all interface and their endpoints
                // the order is if i1 includes e1,r2 and i2 includes e3:
                // c - i1 - e1 - e2 - i2 - e3 
                const uint32_t descriptor_index = 
                    trx->device_request->wValue & 0xFF;
                printf("GET_DESCRIPTOR(configuration) %lu %u\n", 
                        descriptor_index, 
                        trx->device_request->wLength);
                if (descriptor_index == 0)
                {
                    if (trx->device_request->wLength == 
                            sizeof(hal5_usb_configuration_descriptor_t))
                    {
                        // only the configuration descriptor requested
                        
                        hal5_usb_configuration_descriptor_t descriptor;
                        hal5_usb_device_get_configuration_descriptor_ex(
                                descriptor_index,
                                &descriptor);

                        setup_transaction_reply_in(
                                trx,
                                &descriptor,
                                sizeof(descriptor));
                    } 
                    else if (trx->device_request->wLength >= 
                            sizeof(hal5_usb_configuration_descriptor_t))
                    {
                        hal5_usb_configuration_descriptor_t descriptor;
                        hal5_usb_device_get_configuration_descriptor_ex(
                                descriptor_index,
                                &descriptor);

                        setup_transaction_reply_in(
                                trx,
                                &descriptor, 
                                descriptor.wTotalLength);
                    } 
                    else
                    {
                        // this should not happen
                        setup_transaction_stall_in(trx);
                    }
                }
                else
                {
                    setup_transaction_stall_in(trx);
                }
            }
            break;

        case 0x03:
            {
                // string descriptor (optional)
                const uint32_t descriptor_index = 
                    trx->device_request->wValue & 0xFF;

                const uint32_t lang_id = 
                    trx->device_request->wIndex;

                hal5_usb_string_descriptor_t descriptor;

                bool exists = hal5_usb_device_get_string_descriptor_ex(
                        descriptor_index,
                        lang_id,
                        &descriptor);

                if (exists)
                {
                    printf("GET_DESCRIPTOR(string) %lu 0x%04lX\n", 
                            descriptor_index, lang_id);
                    setup_transaction_reply_in(
                            trx,
                            &descriptor,
                            descriptor.bLength);
                }
                else
                {
                    setup_transaction_stall_in(trx);
                }
            }
            break;

        case 0x06:
            {
                // device qualifier descriptor (for HS support)
                printf("GET_DESCRIPTOR(device_qualifier)\n");
                // this is not an HS device
                // so respond with RequestError=stall
                setup_transaction_stall_in(trx);
            }
            break;

        default: 
            {
                setup_transaction_stall_in(trx);
            }
            break;
    }

}

static void device_set_descriptor(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_set_descriptor;

    setup_transaction_stall_out(trx);
}

static void device_get_configuration(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_get_configuration;

    assert (trx->device_request->wValue == 0);
    assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 1);

    uint8_t configuration_value = 0;

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            configuration_value = 
                hal5_usb_device_get_current_configuration_value_ex();
            break;

        case usb_device_state_address:
            configuration_value = 0;
            break;

        default: assert (false);
    }

    setup_transaction_reply_in(trx, &configuration_value, 1);
}

static void device_set_configuration(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_device_set_configuration;

    // upper bytes are reserved and must be zero
    assert ((trx->device_request->wValue & 0xFF00) == 0);
    assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_address:
            break;

        default: assert (false);
    }

    // only lower byte is used
    const uint8_t configuration_value = 
        trx->device_request->wValue & 0xFF;

    printf("SET_CONFIGURATION: %u\n", configuration_value);

    if (hal5_usb_device_set_configuration(configuration_value))
    {
        setup_transaction_reply_in_with_zero(trx);
    }
    else
    {
        setup_transaction_stall_in(trx);
    }
}

static void interface_get_status(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_interface_get_status;

    assert (trx->device_request->wValue == 0);
    //assert (trx->device_request->wIndex == 0);
    assert (trx->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_address: 
            if (TRX_WINDEX_AS_INTERFACE_NUMBER(trx) != 0)
            {
                setup_transaction_stall_in(trx);
                return;
            }
            break;

        default: assert (false);
    }

    // interface status is all reserved to be zero
    uint8_t status[2] = {0};

    setup_transaction_reply_in(trx, status, 2);
}

static void interface_clear_feature(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_interface_clear_feature;

    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
        case usb_device_state_address:
            break;

        default: assert (false);

    }

    // there are no features for interface
    setup_transaction_stall_in(trx);
}

static void interface_set_feature(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_interface_set_feature;

    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
        case usb_device_state_address:
            break;

        default: assert (false);

    }

    // there are no features for interface
    setup_transaction_stall_in(trx);
}

static void interface_get_interface(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_interface_get_interface;

    setup_transaction_stall_in(trx);
}

static void interface_set_interface(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_interface_set_interface;

    setup_transaction_stall_in(trx);
}

static void endpoint_get_status(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_endpoint_get_status;

    assert (trx->device_request->wValue == 0);
    assert (trx->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_address: 
            if (TRX_WINDEX_AS_ENDPOINT_NUMBER(trx) != 0)
            {
                setup_transaction_stall_in(trx);
                return;
            }
            break;

        default: assert (false);
    }

    uint8_t status[2] = {0};

    bool is_halt_set;

    bool success = hal5_usb_device_is_endpoint_halt_set_ex(
            TRX_WINDEX_AS_ENDPOINT_NUMBER(trx), 
            TRX_WINDEX_AS_ENDPOINT_DIR_IN(trx),
            &is_halt_set);

    if (success)
    {
        if (is_halt_set) status[1] |= (1 << 0);
        setup_transaction_reply_in(trx, status, 2);
    }
    else
    {
        setup_transaction_stall_in(trx);
    }
}

static void endpoint_clear_feature(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_endpoint_clear_feature;

    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_address:
            if (TRX_WINDEX_AS_ENDPOINT_NUMBER(trx) != 0) 
            {
                setup_transaction_stall_in(trx);
                return;
            }
            break;

        default: assert (false);

    }

    const uint16_t feature_selector = 
        trx->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_ENDPOINT_HALT)
    {
        success = hal5_usb_device_clear_endpoint_halt_ex(
                TRX_WINDEX_AS_ENDPOINT_NUMBER(trx), 
                TRX_WINDEX_AS_ENDPOINT_DIR_IN(trx));
    }

    if (success) setup_transaction_reply_in_with_zero(trx);
    else setup_transaction_stall_in(trx);

}

static void endpoint_set_feature(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_endpoint_set_feature;

    assert (trx->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_address:
            if (TRX_WINDEX_AS_ENDPOINT_NUMBER(trx) != 0) 
            {
                setup_transaction_stall_in(trx);
                return;
            }
            break;

        default: assert (false);

    }

    const uint16_t feature_selector = trx->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_ENDPOINT_HALT)
    {
        success = hal5_usb_device_set_endpoint_halt_ex(
                TRX_WINDEX_AS_ENDPOINT_NUMBER(trx), 
                TRX_WINDEX_AS_ENDPOINT_DIR_IN(trx));
    }

    if (success) setup_transaction_reply_in_with_zero(trx);
    else setup_transaction_stall_in(trx);
}

static void endpoint_synch_frame(
        hal5_usb_transaction_t *trx)
{
    standard_request = standard_request_endpoint_synch_frame;

    assert (trx->device_request->wValue == 0);
    assert (trx->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_address: 
            setup_transaction_stall_in(trx);
            return;

        default: assert (false);
    }

    uint16_t frame_number;
    
    if (hal5_usb_device_get_synch_frame_ex(
            TRX_WINDEX_AS_ENDPOINT_NUMBER(trx),
            TRX_WINDEX_AS_ENDPOINT_DIR_IN(trx),
            &frame_number))
    {
        setup_transaction_reply_in(trx, &frame_number, 2);
    }
    else
    {
        setup_transaction_stall_in(trx);
    }
} 

void hal5_usb_device_setup_transaction_completed_ep0(
        hal5_usb_transaction_t *trx)
{
    assert (trx->ea == 0);
    assert (trx->rx_data_len == 8);

    trx->device_request = 
        (hal5_usb_device_request_t*) trx->rx_data;

    printf("S 0x%02X 0x%02X 0x%04X 0x%04X 0x%04X\n", 
            trx->device_request->bmRequestType, 
            trx->device_request->bRequest,
            trx->device_request->wValue, 
            trx->device_request->wIndex, 
            trx->device_request->wLength);

    // when a SETUP transaction arrives
    // processing re-starts, no previous state affects this
    // standard_request is set in individual functions
    standard_request = standard_request_null;

    switch (trx->device_request->bmRequestType)
    {
        // recipient = DEVICE
        // data transfer direction = HOST TO DEVICE
        case 0x00:
            {
                switch (trx->device_request->bRequest)
                {
                    case 0x01: device_clear_feature(trx); break;
                    case 0x03: device_set_feature(trx); break;
                    case 0x05: device_set_address(trx); break;
                    case 0x07: device_set_descriptor(trx); break;
                    case 0x09: device_set_configuration(trx); break;
                }
            } 
            break;

        // data transfer direction = DEVICE TO HOST
        case 0x80:
            {
                switch (trx->device_request->bRequest)
                {
                    case 0x00: device_get_status(trx); break;
                    case 0x06: device_get_descriptor(trx); break;
                    case 0x08: device_get_configuration(trx); break;
                }
            } 
            break;

        // recipient = INTERFACE
        // data transfer direction = HOST TO DEVICE
        case 0x01:
            {
                switch (trx->device_request->bRequest)
                {
                    case 0x01: interface_clear_feature(trx); break;
                    case 0x03: interface_set_feature(trx); break;
                    case 0x11: interface_set_interface(trx); break;
                }
            } 
            break;


        // data transfer direction = DEVICE TO HOST
        case 0x81:
            {
                switch (trx->device_request->bRequest)
                {
                    case 0x00: interface_get_status(trx); break;
                    case 0x0A: interface_get_interface(trx); break;
                }
            } 
            break;

        // recipient = ENDPOINT
        // data transfer direction = HOST TO DEVICE
        case 0x02:
            {
                switch (trx->device_request->bRequest)
                {
                    case 0x01: endpoint_clear_feature(trx); break;
                    case 0x03: endpoint_set_feature(trx); break;
                }
            } 
            break;

        // data transfer direction = DEVICE TO HOST
        case 0x82:
            {
                switch (trx->device_request->bRequest)
                {
                    case 0x00: endpoint_get_status(trx); break;
                    case 0x12: endpoint_synch_frame(trx); break;
                }
            } 
            break;

    }

    // either it is not catched by any case above
    // or I have forgotten to set standard_request in individual functions
    if (standard_request == standard_request_null) 
    {
        printf("unknown standard request: bmRequestType: 0x%02X, bRequest: 0x%02X\n", 
                trx->device_request->bmRequestType,
                trx->device_request->bRequest);
        assert (false);
    }
}

void hal5_usb_device_out_transaction_completed_ep0(
        hal5_usb_transaction_t *trx)
{
    assert (trx->ea == 0);

    switch (standard_request)
    {
        // first SETUP transaction should complete 
        case standard_request_null:
            assert (false);
            break;

        case standard_request_device_get_status:
            setup_transaction_finished(trx);
            break;

        case standard_request_device_clear_feature:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_device_set_feature:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_device_set_address:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_device_get_descriptor:
            setup_transaction_finished(trx);
            break;

        case standard_request_device_set_descriptor:
            // not supported, stalled before
            // this cannot happen
            assert (false);
            break;

        case standard_request_device_get_configuration:
            setup_transaction_finished(trx);
            break;

        case standard_request_device_set_configuration:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_interface_get_status:
            setup_transaction_finished(trx);
            break;

        case standard_request_interface_clear_feature:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_interface_set_feature:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_interface_get_interface:
            setup_transaction_finished(trx);
            break;

        case standard_request_interface_set_interface:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_endpoint_get_status:
            setup_transaction_finished(trx);
            break;

        case standard_request_endpoint_clear_feature:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_endpoint_set_feature:
            // cannot have OUT
            // it is SETUP IN_0
            assert (false);
            break;

        case standard_request_endpoint_synch_frame:
            setup_transaction_finished(trx);
            break;
    }
}

void hal5_usb_device_in_transaction_completed_ep0(
        hal5_usb_transaction_t *trx)
{
    assert (trx->ea == 0);

    switch (standard_request)
    {
        // first SETUP transaction should complete 
        case standard_request_null:
            assert (false);
            break;

        case standard_request_device_get_status:
            setup_transaction_ack_out_zero(trx);
            break;

        case standard_request_device_clear_feature:
            setup_transaction_finished(trx);
            break;

        case standard_request_device_set_feature:
            setup_transaction_finished(trx);
            break;

        case standard_request_device_set_address:
            // ATTENTION
            // device address has to be set here
            // it is going to be used 
            //  after confirming Set Address with IN len=0
            // this is different than all other standard requests
            // that takes effect immediately
            hal5_usb_device_set_address(device_address);
            setup_transaction_finished(trx);
            break;

        case standard_request_device_get_descriptor:
            setup_transaction_ack_out_zero(trx);
            break;

        case standard_request_device_set_descriptor:
            setup_transaction_finished(trx);
            break;

        case standard_request_device_get_configuration:
            setup_transaction_ack_out_zero(trx);
            break;

        case standard_request_device_set_configuration:
            setup_transaction_finished(trx);
            break;

        case standard_request_interface_get_status:
            setup_transaction_ack_out_zero(trx);
            break;

        case standard_request_interface_clear_feature:
            setup_transaction_finished(trx);
            break;

        case standard_request_interface_set_feature:
            setup_transaction_finished(trx);
            break;

        case standard_request_interface_get_interface:
            setup_transaction_ack_out_zero(trx);
            break;

        case standard_request_interface_set_interface:
            setup_transaction_finished(trx);
            break;

        case standard_request_endpoint_get_status:
            setup_transaction_ack_out_zero(trx);
            break;

        case standard_request_endpoint_clear_feature:
            setup_transaction_finished(trx);
            break;

        case standard_request_endpoint_set_feature:
            setup_transaction_finished(trx);
            break;

        case standard_request_endpoint_synch_frame:
            setup_transaction_ack_out_zero(trx);
            break;
    }
}
