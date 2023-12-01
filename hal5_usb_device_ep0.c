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
#include <stdlib.h>
#include <string.h>

#include "hal5.h"

#define WINDEX_AS_ENDPOINT_NUMBER(ep) \
    ((uint8_t) (ep->device_request->wIndex & 0x000F))

#define WINDEX_AS_ENDPOINT_DIR_IN(ep) \
    ((bool) (ep->device_request->wIndex & 0x0080))

#define WINDEX_AS_INTERFACE_NUMBER(ep) \
    ((uint8_t) (ep->device_request->wIndex & 0x00FF))

// temporary storage of device_address
// stored at SETUP of SET ADDRESS
// used at IN_0 of SET ADDRESS
static uint8_t device_address = 0;

// https://github.com/pbatard/libwdi/wiki/WCID-Devices
// implemented as a string descriptor 
// including vendor code and padding at the end of bString
static const hal5_usb_string_descriptor_t microsoft_os_string_descriptor = 
{
    0x12,
    0x03,
    // MSFT100 and byte vendor code and byte padding (00)
    {0x4D, 0x00, 0x53, 0x00, 0x46, 0x00, 0x54, 0x00, 0x31, 0x00, 0x31, 0x00, 0x30, 0x00, 0x11, 0x00}
};

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

// used for temporarily storing current standard request in processing
// stored at SETUP, used in following IN and/or OUT transactions
static usb_standard_request_t standard_request = standard_request_null;

// this is called when a standard request is finished
// it can be any type of standard request
// sometimes called after IN, sometimes OUT
// basically it should prepare for new RX
void standard_request_completed(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_null;

    hal5_usb_ep_set_status(
            ep,
            ep_status_valid,
            ep_status_disabled);
}

// product_string_version becomes " vXX.YY"
// where XX is major and YY is minor device version 
// queried from the device implementation
// 7 chars in UTF-16 is 14 bytes
static uint8_t product_string_version[14] = {0};

// initializes product_string_version
static void initialize_product_string_version()
{
    uint8_t major = hal5_usb_device_version_major_ex();
    if (major > 99) major = 99;

    uint8_t minor = hal5_usb_device_version_minor_ex();
    if (minor > 99) minor = 99;

    uint32_t off = 0;

    product_string_version[off] = ' '; 
    off+=2;

    product_string_version[off] = 'v'; 
    off+=2;

    if ((major / 10) > 0)
    {
        product_string_version[off] = (major / 10) + 48;
        off+=2;
    }

    product_string_version[off] = (major % 10) + 48; 
    off+=2;

    product_string_version[off] = '.'; 
    off+=2;

    if ((minor / 10) > 0)
    {
        product_string_version[off] = (minor / 10) + 48;
        off+=2;
    }

    product_string_version[off] = (minor % 10) + 48;
    off+=2;

    assert (off <= 14);
}


// SETUP TRANSACTION HELPERS

// SETUP IN_DATA OUT_0 e.g. get_descriptor
// this is used to send data for IN
static void setup_transaction_reply_in(
        hal5_usb_endpoint_t* ep,
        const void* data, 
        const size_t len,
        const size_t expected)
{   
    hal5_usb_ep_prepare_for_in(
            ep,
            ep_status_stall,
            data,
            len,
            true,
            expected);
}

// SETUP IN_DATA OUT_0 e.g. get_descriptor
// this is used to acknowledge the OUT with zero data
static void setup_transaction_ack_out_zero(
        hal5_usb_endpoint_t* ep)
{
    hal5_usb_ep_prepare_for_out(ep, ep_status_stall);
}

// SETUP IN_0 e.g. clear_feature
// SETUP OUT_DATA IN_0 e.g. set_descriptor

// this is used to confirm the request by sending a zero size data for in 
static void setup_transaction_reply_in_with_zero(
        hal5_usb_endpoint_t* ep)
{
    hal5_usb_ep_prepare_for_in(
            ep,
            ep_status_stall,
            NULL,
            0,
            false,
            0);
}

// USB 2.0 9.2.7 Request Error
// The device deals with the Request Error by returning
// a STALL PID in response to the next Data stage transaction
// or in the Status stage of the message.
// It is preferred that the STALL PID be returned at the next
// Data stage transaction, as this avoids unnecessary bus activity.

// RequestError is signalled with STALL
// STALL any transaction (IN or OUT) 
static void setup_transaction_stall(
        hal5_usb_endpoint_t* ep)
{
    hal5_usb_ep_set_status(
            ep,
            ep_status_valid,
            ep_status_stall);
}

// STANDARD REQUESTS

static void device_get_status(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_get_status;

    assert (ep->device_request->wValue == 0);
    assert (ep->device_request->wIndex == 0);
    assert (ep->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
        case usb_device_state_addressed: 
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

    setup_transaction_reply_in(
            ep, 
            status, 
            2,
            ep->device_request->wLength);
}

static void device_clear_feature(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_clear_feature;

    assert (ep->device_request->wIndex == 0);
    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
        case usb_device_state_addressed: 
            break;
        default: assert (false);
    }

    const uint16_t feature_selector = 
        ep->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP) 
    {
        success = hal5_usb_device_clear_device_remote_wakeup_ex();
    }
    else if (feature_selector == FEATURE_SELECTOR_TEST_MODE)
    {
        // test mode cannot be cleared by Clear Feature
        setup_transaction_stall(ep);
    }

    if (success) setup_transaction_reply_in_with_zero(ep);
    else setup_transaction_stall(ep);
}

static void device_set_feature(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_set_feature;

    assert (ep->device_request->wIndex == 0);
    assert (ep->device_request->wLength == 0);

    const uint16_t feature_selector = 
        ep->device_request->wValue;

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
        case usb_device_state_addressed: 
            break;

        // 9.4.9 Set Feature
        // test mode can also be set in default state
        case usb_device_state_default:
            assert (feature_selector == FEATURE_SELECTOR_TEST_MODE);
            break;

        default: assert (false);
    }

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP) 
    {
        success = hal5_usb_device_set_device_remote_wakeup_ex();
    }
    else if (feature_selector == FEATURE_SELECTOR_TEST_MODE)
    {
        success = hal5_usb_device_set_test_mode_ex();
    }

    if (success) setup_transaction_reply_in_with_zero(ep);
    else setup_transaction_stall(ep);
}

static void device_set_address(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_set_address;

    assert (ep->device_request->wValue <= 127);
    assert (ep->device_request->wIndex == 0);
    assert (ep->device_request->wLength == 0);

    // it is possible that this request 
    // comes again in address state
    // or comes with address=0
    // see hal5_usb_device_set_address method
    // for more information

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_default: 
        case usb_device_state_addressed: 
            break;

        default: assert (false);
    }


    // IMPORTANT: address is actually set in Status stage
    // this is different than all other requests
    // device_address here is a temporary storage
    
    device_address = ep->device_request->wValue;

    setup_transaction_reply_in_with_zero(ep);
}

static void device_get_descriptor(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_get_descriptor;

    const uint32_t descriptor_type = 
        (ep->device_request->wValue >> 8) & 0xFF;
    
    switch (descriptor_type)
    {
        case 0x01:
            {
                // device descriptor (device should have one, must)
                const hal5_usb_device_descriptor_t* const dd = hal5_usb_device_descriptor;

                // the issue is, just after bus reset in default state
                // get_descriptor for device is issued with a large wLength
                // but the expectation is only to read max packet size
                // so if packet size is small (8 bytes), 
                //  even if wLength is 64 (because it can be with FS and HS)
                // actually after the first packet, host completes the request
                // with an OUT_0, for this to work, RX should be enabled
                // so I change the len to a single byte if in default state
                // so the code in hal5_usb_device runs as expected
                // for all other requests host behaves as expected 
                // (waits until all data is sent)
                uint8_t len = dd->bLength;

                if (hal5_usb_device_get_state() == 
                        usb_device_state_default)
                {
                    len = ep->mps;
                }

                setup_transaction_reply_in(
                        ep, 
                        dd, 
                        len,
                        ep->device_request->wLength);
            }
            break;

        case 0x02:
            {
                // configuration descriptor (device should have at least one)
                // according to USB 2.0 spec:
                //   when the host requests the configuration descriptor
                //   all related interface and 
                //   endpoint descriptors are returned
                // the order is if i1 includes e1,r2 and i2 includes e3:
                // c - i1 - e1 - e2 - i2 - e3 
                // HOWEVER, host usually requests this in two requests
                // one with wLength == configuration descriptor bLength
                // and then
                // one with wLength == configuration descriptor wTotalLength
                const uint32_t configuration_descriptor_index = 
                    ep->device_request->wValue & 0xFF;

                const hal5_usb_device_descriptor_t* const dd = hal5_usb_device_descriptor;

                if (configuration_descriptor_index < dd->bNumConfigurations)
                {
                    // copy all chain (conf-interface*-endpoint*)
                    // to a temporary buffer
                    // then use only wLength part
                    // this covers requests only for configuration
                    // or more
                    const hal5_usb_configuration_descriptor_t* cd = 
                        dd->configurations[configuration_descriptor_index];

                    uint8_t tmp[256];
                    uint8_t offset = 0;
                    memcpy(tmp+offset, cd, cd->bLength);
                    offset += cd->bLength;

                    for (uint8_t i = 0; i < cd->bNumInterfaces; i++)
                    {
                        const hal5_usb_interface_descriptor_t* id = 
                            cd->interfaces[i];
                        memcpy(tmp+offset, id, id->bLength);
                        offset += id->bLength;

                        for (uint8_t k = 0; k < id->bNumEndpoints; k++)
                        {
                            const hal5_usb_endpoint_descriptor_t* ed = 
                                id->endpoints[k];
                            memcpy(tmp+offset, ed, ed->bLength);
                            offset += ed->bLength;
                        }
                    }

                    // just a precaution
                    // this is impossible to happen 
                    // when create_descriptors.py is used
                    assert (cd->wTotalLength == offset);

                    // some hosts (linux):
                    // - first requests configuration descriptor only with wLength=9 
                    // - then it requests all with wLength=wTotalLength
                    //
                    // some hosts (windows):
                    // - requests all at one call with wLength=255 > wTotalLength
                    setup_transaction_reply_in(
                                ep,
                                tmp,
                                cd->wTotalLength,
                                ep->device_request->wLength);
                }
                else
                {
                    // no such descriptor
                    setup_transaction_stall(ep);
                }
            }
            break;

        case 0x03:
            {
                // string descriptor (optional)
                const uint32_t string_descriptor_index = 
                    ep->device_request->wValue & 0xFF;

                const uint32_t lang_id = 
                    ep->device_request->wIndex;

                // 0xEE is the microsoft OS string descriptor location
                if (string_descriptor_index == 0xEE)
                {
                    setup_transaction_reply_in(
                            ep,
                            &microsoft_os_string_descriptor,
                            microsoft_os_string_descriptor.bLength,
                            ep->device_request->wLength);
                }
                else if (string_descriptor_index < hal5_usb_number_of_string_descriptors)
                {
                    const hal5_usb_string_descriptor_t* const sd = 
                        hal5_usb_string_descriptors[string_descriptor_index];

                    if (hal5_usb_product_string_append_version &&
                            (string_descriptor_index == 
                             hal5_usb_device_descriptor->iProduct))
                    {
                        // initialize the string if not initialized before
                        if (product_string_version[0] == 0)
                        {
                            initialize_product_string_version();
                        }
// do not warn for const qualifier for memcpy
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdiscarded-qualifiers"
                        // copy the product version at the end
                        // if the length of product_string_version changes
                        // this code does not need to change
                        memcpy(&(sd->bString[sd->bLength-2-sizeof(product_string_version)/sizeof(uint8_t)]),
                                product_string_version,
                                sizeof(product_string_version)/sizeof(uint8_t));
#pragma GCC diagnostic pop
                    }

                    setup_transaction_reply_in(
                            ep,
                            sd,
                            sd->bLength,
                            ep->device_request->wLength);
                }
                else
                {
                    setup_transaction_stall(ep);
                }
            }
            break;

        case 0x06:
            {
                // device qualifier descriptor (for HS support)
                // according to USB 2.0 spec:
                //   if a full-speed only device (with version=0200)
                //   receives this request, 
                //   it must respond with a request error.
                // RequestError=stall
            }
            // device qualifier descriptor is handled 
            // as same as any other unknown descriptors
            // by returning a stall
        default:
            {
                setup_transaction_stall(ep);
            }
            break;
    }

}

static void device_set_descriptor(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_set_descriptor;

    setup_transaction_stall(ep);
}

static void device_get_configuration(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_get_configuration;

    assert (ep->device_request->wValue == 0);
    assert (ep->device_request->wIndex == 0);
    assert (ep->device_request->wLength == 1);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_addressed:
            break;

        default: assert (false);
    }

    uint8_t configuration_value = hal5_usb_device_get_configuration_value();

    setup_transaction_reply_in(
            ep, 
            &configuration_value, 
            1,
            ep->device_request->wLength);
}

static void device_set_configuration(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_device_set_configuration;

    // upper bytes are reserved and must be zero
    assert ((ep->device_request->wValue & 0xFF00) == 0);
    assert (ep->device_request->wIndex == 0);
    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_addressed:
            break;

        default: assert (false);
    }

    // only lower byte is used
    const uint8_t configuration_value = 
        ep->device_request->wValue & 0xFF;

    bool success = hal5_usb_device_set_configuration_value(configuration_value);

    if (success) setup_transaction_reply_in_with_zero(ep);
    else setup_transaction_stall(ep);
}

static void interface_get_status(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_interface_get_status;

    assert (ep->device_request->wValue == 0);
    assert (ep->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_addressed: 
            if (WINDEX_AS_INTERFACE_NUMBER(ep) != 0)
            {
                setup_transaction_stall(ep);
                return;
            }
            break;

        default: assert (false);
    }

    // interface status is all reserved to be zero
    uint8_t status[2] = {0};

    setup_transaction_reply_in(
            ep, 
            status, 
            2,
            ep->device_request->wLength);
}

static void interface_clear_feature(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_interface_clear_feature;

    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
        case usb_device_state_addressed:
            break;

        default: assert (false);

    }

    // there are no features for interface
    setup_transaction_stall(ep);
}

static void interface_set_feature(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_interface_set_feature;

    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
        case usb_device_state_addressed:
            break;

        default: assert (false);

    }

    // there are no features for interface
    setup_transaction_stall(ep);
}

static void interface_get_interface(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_interface_get_interface;

    assert (ep->device_request->wValue == 0);
    assert ((ep->device_request->wIndex & 0xFF00) == 0);
    assert (ep->device_request->wLength == 1);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_addressed: 
            setup_transaction_stall(ep);
            return;

        default: assert (false);
    }

    uint8_t alternate_setting;

    bool success = hal5_usb_device_get_interface_ex(
            WINDEX_AS_INTERFACE_NUMBER(ep),
            &alternate_setting);

    if (success) 
    {
        setup_transaction_reply_in(
                ep, 
                &alternate_setting, 
                1,
                ep->device_request->wLength);
    }
    else setup_transaction_stall(ep);
}

static void interface_set_interface(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_interface_set_interface;

    // this does not write in the spec
    // but bAlternateSetting is a byte, so I assume it like this
    assert ((ep->device_request->wValue & 0xFF00) == 0);
    assert (ep->device_request->wIndex <= 127);
    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_addressed:
            setup_transaction_stall(ep);
            return;

        default: assert (false);
    }

    const uint8_t alternate_setting = 
        ep->device_request->wValue & 0xFF;

    bool success = hal5_usb_device_set_interface_ex(
            WINDEX_AS_INTERFACE_NUMBER(ep),
            alternate_setting);

    if (success) setup_transaction_reply_in_with_zero(ep);
    else setup_transaction_stall(ep);
}

static void endpoint_get_status(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_endpoint_get_status;

    assert (ep->device_request->wValue == 0);
    assert (ep->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_addressed: 
            if (WINDEX_AS_ENDPOINT_NUMBER(ep) != 0)
            {
                setup_transaction_stall(ep);
                return;
            }
            break;

        default: assert (false);
    }

    uint8_t status[2] = {0};

    bool is_halt_set;

    bool success = hal5_usb_device_is_endpoint_halt_set_ex(
            WINDEX_AS_ENDPOINT_NUMBER(ep), 
            WINDEX_AS_ENDPOINT_DIR_IN(ep),
            &is_halt_set);

    if (success)
    {
        if (is_halt_set) status[1] |= (1 << 0);
        setup_transaction_reply_in(
                ep, 
                status, 
                2,
                ep->device_request->wLength);
    }
    else
    {
        setup_transaction_stall(ep);
    }
}

static void endpoint_clear_feature(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_endpoint_clear_feature;

    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_addressed:
            if (WINDEX_AS_ENDPOINT_NUMBER(ep) != 0) 
            {
                setup_transaction_stall(ep);
                return;
            }
            break;

        default: assert (false);

    }

    const uint16_t feature_selector = 
        ep->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_ENDPOINT_HALT)
    {
        success = hal5_usb_device_clear_endpoint_halt_ex(
                WINDEX_AS_ENDPOINT_NUMBER(ep), 
                WINDEX_AS_ENDPOINT_DIR_IN(ep));
    }

    if (success) setup_transaction_reply_in_with_zero(ep);
    else setup_transaction_stall(ep);

}

static void endpoint_set_feature(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_endpoint_set_feature;

    assert (ep->device_request->wLength == 0);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured:
            break;

        case usb_device_state_addressed:
            if (WINDEX_AS_ENDPOINT_NUMBER(ep) != 0) 
            {
                setup_transaction_stall(ep);
                return;
            }
            break;

        default: assert (false);

    }

    const uint16_t feature_selector = ep->device_request->wValue;

    bool success = false;

    if (feature_selector == FEATURE_SELECTOR_ENDPOINT_HALT)
    {
        success = hal5_usb_device_set_endpoint_halt_ex(
                WINDEX_AS_ENDPOINT_NUMBER(ep), 
                WINDEX_AS_ENDPOINT_DIR_IN(ep));
    }

    if (success) setup_transaction_reply_in_with_zero(ep);
    else setup_transaction_stall(ep);
}

static void endpoint_synch_frame(
        hal5_usb_endpoint_t* ep)
{
    standard_request = standard_request_endpoint_synch_frame;

    assert (ep->device_request->wValue == 0);
    assert (ep->device_request->wLength == 2);

    switch (hal5_usb_device_get_state())
    {
        case usb_device_state_configured: 
            break;

        case usb_device_state_addressed: 
            setup_transaction_stall(ep);
            return;

        default: assert (false);
    }

    uint16_t frame_number;
    
    bool success = hal5_usb_device_get_synch_frame_ex(
            WINDEX_AS_ENDPOINT_NUMBER(ep),
            WINDEX_AS_ENDPOINT_DIR_IN(ep),
            &frame_number);

    if (success) 
    {
        setup_transaction_reply_in(
                ep, 
                &frame_number, 
                2,
                ep->device_request->wLength);
    }
    else setup_transaction_stall(ep);
} 

static const char* bRequestLabels[] =
{
    "GET_STATUS",
    "CLEAR_FEATURE",
    "reserved",
    "SET_FEATURE",
    "reserved",
    "SET_ADDRESS",
    "GET_DESCRIPTOR",
    "SET_DESCRIPTOR",
    "GET_CONFIGURATION",
    "SET_CONFIGURATION",
    "GET_INTERFACE",
    "SET_INTERFACE",
    "SYNCH_FRAME",
};

static const char* bmRequestTypeRecipientLabels[] =
{
    "Device",
    "Interface",
    "Endpoint",
    "Other"
};

static const char* wValueDescriptorTypes[] =
{
    "",
    "Device",
    "Configuration",
    "String",
    "Interface",
    "Endpoint",
    "Device_Qualifier",
    "Other_Speed_Configuration",
    "Interface_Power"
};

void hal5_usb_device_setup_transaction_completed_ep0(
        hal5_usb_endpoint_t* ep)
{
    assert (ep->endp == 0);
    assert (ep->device_request != NULL);

    // log all data of SETUP
    CONSOLE("S 0x%02X 0x%02X 0x%04X 0x%04X 0x%04X\n", 
            ep->device_request->bmRequestType, 
            ep->device_request->bRequest,
            ep->device_request->wValue, 
            ep->device_request->wIndex, 
            ep->device_request->wLength);

    // log the request type and recipient
    CONSOLE("%s.%s",
            bRequestLabels[
                ep->device_request->bRequest],
            bmRequestTypeRecipientLabels[
                ep->device_request->bmRequestType & 0x1F]);

    // log GET_DESCRIPTOR parameters
    if (ep->device_request->bRequest == 0x06)
    {
        CONSOLE(".%s (%u)\n",
                wValueDescriptorTypes[ep->device_request->wValue >> 8],
                ep->device_request->wValue & 0xFF);
    }
    // log SET_ADDRESS parameters
    else if (ep->device_request->bRequest == 0x05)
    {
        CONSOLE(" (%u)\n", ep->device_request->wValue);
    }
    else
    {
        CONSOLE("\n");
    }

    // when a SETUP transaction arrives
    // processing re-starts, no previous state affects this
    // standard_request is set in individual functions
    standard_request = standard_request_null;

    switch (ep->device_request->bmRequestType)
    {
        // recipient = DEVICE
        // data transfer direction = HOST TO DEVICE
        case 0x00:
            {
                switch (ep->device_request->bRequest)
                {
                    case 0x01: device_clear_feature(ep); break;
                    case 0x03: device_set_feature(ep); break;
                    case 0x05: device_set_address(ep); break;
                    case 0x07: device_set_descriptor(ep); break;
                    case 0x09: device_set_configuration(ep); break;
                }
            } 
            break;

        // recipient = DEVICE
        // data transfer direction = DEVICE TO HOST
        case 0x80:
            {
                switch (ep->device_request->bRequest)
                {
                    case 0x00: device_get_status(ep); break;
                    case 0x06: device_get_descriptor(ep); break;
                    case 0x08: device_get_configuration(ep); break;
                }
            } 
            break;

        // recipient = INTERFACE
        // data transfer direction = HOST TO DEVICE
        case 0x01:
            {
                switch (ep->device_request->bRequest)
                {
                    case 0x01: interface_clear_feature(ep); break;
                    case 0x03: interface_set_feature(ep); break;
                    case 0x11: interface_set_interface(ep); break;
                }
            } 
            break;

        // recipient = INTERFACE
        // data transfer direction = DEVICE TO HOST
        case 0x81:
            {
                switch (ep->device_request->bRequest)
                {
                    case 0x00: interface_get_status(ep); break;
                    case 0x0A: interface_get_interface(ep); break;
                }
            } 
            break;

        // recipient = ENDPOINT
        // data transfer direction = HOST TO DEVICE
        case 0x02:
            {
                switch (ep->device_request->bRequest)
                {
                    case 0x01: endpoint_clear_feature(ep); break;
                    case 0x03: endpoint_set_feature(ep); break;
                }
            } 
            break;

        // recipient = ENDPOINT
        // data transfer direction = DEVICE TO HOST
        case 0x82:
            {
                switch (ep->device_request->bRequest)
                {
                    case 0x00: endpoint_get_status(ep); break;
                    case 0x12: endpoint_synch_frame(ep); break;
                }
            } 
            break;

    }

    // either it is not catched by any case above
    // or I have forgotten to set standard_request in individual functions
    if (standard_request == standard_request_null)
    {
        CONSOLE("unknown standard request: bmRequestType: 0x%02X, bRequest: 0x%02X\n", 
                ep->device_request->bmRequestType,
                ep->device_request->bRequest);

        setup_transaction_stall(ep);
    }
}

void hal5_usb_device_out_stage_completed_ep0(
        hal5_usb_endpoint_t* ep)
{
    assert (ep->endp == 0);

    CONSOLE("O\n");

    switch (standard_request)
    {
        // first SETUP transaction should complete 
        case standard_request_null:
            assert (false);
            break;

        // SETUP IN_0
        // these requests cannot have OUT transactions
        case standard_request_device_clear_feature:
        case standard_request_device_set_feature:
        case standard_request_interface_clear_feature:
        case standard_request_interface_set_feature:
        case standard_request_endpoint_clear_feature:
        case standard_request_endpoint_set_feature:
        case standard_request_device_set_address:
        case standard_request_device_set_configuration:
        case standard_request_interface_set_interface:
            assert (false);
            break;

        // SETUP IN OUT_0
        case standard_request_device_get_status:
        case standard_request_interface_get_status:
        case standard_request_endpoint_get_status:
        case standard_request_interface_get_interface:
        case standard_request_device_get_descriptor:
        case standard_request_device_get_configuration:
        case standard_request_endpoint_synch_frame:
            standard_request_completed(ep);
            break;

        // SETUP OUT IN_0
        case standard_request_device_set_descriptor:
            // not supported
            // TX STALLed after setup
            // this cannot happen
            assert (false);
            break;
    }
}

void hal5_usb_device_in_stage_completed_ep0(
        hal5_usb_endpoint_t* ep)
{
    assert (ep->endp == 0);

    CONSOLE("I\n");

    switch (standard_request)
    {
        // first SETUP transaction should complete 
        case standard_request_null:
            assert (false);
            break;

        // SETUP IN_0
        case standard_request_device_clear_feature:
        case standard_request_device_set_feature:
        case standard_request_interface_clear_feature:
        case standard_request_interface_set_feature:
        case standard_request_endpoint_clear_feature:
        case standard_request_endpoint_set_feature:
        case standard_request_device_set_configuration:
        case standard_request_interface_set_interface:
            standard_request_completed(ep);
            break;

        // SETUP IN_0 (like above)
        // however the request is processed at the end of status
        // this is special for SET_ADDRESS request
        // because the same address should be used in its status stage
        case standard_request_device_set_address:
            // ATTENTION
            // device address has to be set here
            // it is going to be used 
            //  after confirming Set Address with IN len=0
            // this is different than all other standard requests
            // that takes effect immediately
            assert (device_address != 0);
            hal5_usb_device_set_address(device_address);
            device_address = 0;
            standard_request_completed(ep);
            break;

        // SETUP IN OUT_0
        case standard_request_device_get_status:
        case standard_request_interface_get_status:
        case standard_request_endpoint_get_status:
        case standard_request_device_get_descriptor:
        case standard_request_device_get_configuration:
        case standard_request_interface_get_interface:
        case standard_request_endpoint_synch_frame:
            setup_transaction_ack_out_zero(ep);
            break;

        // SETUP OUT IN_0
        case standard_request_device_set_descriptor:
            standard_request_completed(ep);
            break;
    }
}
