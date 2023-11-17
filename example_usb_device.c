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

#include <string.h>

#include "hal5_usb_device.h"

static hal5_usb_device_descriptor_t dd = 
{
    18,         // size of this descriptor, always 18
    0x01,       // type=Device Descriptor=0x01
    0x0200,     // highest supported USB version, bcd encoded high and low 
    0x00,       // device class
    0x00,       // device sub class
    0x00,       // device protocol
    64,         // max packet size for zero endpoint, 8, 16, 32 or 64 bytes
    0x1209,     // pid.codes VID
    0x0001,     // pid.codes test PID
    0x0100,     // device version, bcd format, assigned by the developer
    1,          // index of manufacturer string descriptor
    2,          // index of product string descriptor
    3,          // index of serial number string descriptor
    1           // number of configurations
};

static hal5_usb_device_qualifier_descriptor_t dqd = 
{
    10,         // size of this descriptor, always 10
    0x06,       // type=Device Qualifier Descriptor=0x06
    0x0200,     // USB spec version 2.0
    0x00,       // device class code
    0x00,       // device subclass code
    0x00,       // protocol code
    0,          // maximum packet size for other speed
    0,          // number of other-speed configurations
    0           // reserved, must be 0
};

static hal5_usb_configuration_descriptor_t c1d = 
{
    9,          // size of this descriptor, always 9
    0x02,       // type=Configuration Descriptor=0x02
    25,         // 2 bytes total length of configuration+interface+endpoint descriptors
                // this value is 9+9+7=25 for one descriptor each
    1,          // number of interfaces
    0,          // argument used with SetConfiguration to select this configuration
    4,          // index of string descriptor
    0xC0,       // attributes, D6 is self powered
    0           // maximum power, bus powered
};

static hal5_usb_interface_descriptor_t c1i1d = 
{
    9,      // size of this descriptor, always 9
    0x04,   // type=Interface Descriptor=0x04
    1,      // interface number
    0,      // argument used to select alternative setting
    1,      // number of endpoints excluding the control endpoint 0
    0,      // class code (assigned by usb.org)
    0,      // subclass code (assigned by usb.org)
    0,      // protocol code (assigned by usb.org)
    5       // index of string descriptor
};

static hal5_usb_endpoint_descriptor_t c1i1e1d = 
{
    7,      // size of this descriptor, always 7
    0x05,   // type=Endpoint Descriptor=0x05
    1,      // endpoint address, includes direction for iso, bulk and interrupt
    0,      // transfer type=control
    64,     // 2 bytes maximum packet size
    0       // interval fo polling endpoint data transfers in frame count units
            // ignored for bulk and control endpoints
            // iso endpoints has to set 1 here
            // interrupt endpoints has to choose 1 to 255
};

static hal5_usb_string_descriptor_t sds[] = 
{
    {
        4,              // size of descriptor
        0x03,           // descriptor type=String Descriptor=0x03
        {0x09, 0x04},   // English-US, USB LANGID, write reverse (0x0409)
    },
    {
        2+18,           // 18 is the length of string, not null terminated
        0x03,
        {'m', 0, 'e', 0, 't', 0, 'e', 0, 'b', 0, 'a', 0, 'l', 0, 'c', 0, 'i', 0}
    },
    {
        2+8,
        0x03,
        {'H', 0, 'A', 0, 'L', 0, '5', 0}
    },
    {
        2+6,
        0x03,
        {'0', 0, '0', 0, '7', 0}
    },
    {
        2+8,
        0x03,
        {'c', 0, 'o', 0, 'n', 0, 'f', 0}
    },
    {
        2+10,
        0x03,
        {'i', 0, 'f', 0, 'a', 0, 'c', 0, 'e'}
    }
};

void hal5_usb_device_get_device_descriptor_ex(
        void* descriptor) 
{
    memcpy(descriptor, &dd, sizeof(dd));
}

bool hal5_usb_device_get_configuration_descriptor_ex(
        uint8_t configuration,
        void* descriptor)
{
    if (configuration > 0) return false;

    memcpy(descriptor, &c1d, sizeof(c1d));

    return true;
}

bool hal5_usb_device_get_interface_descriptor_ex(
        uint8_t configuration,
        uint8_t interface,
        void* descriptor)
{
    if (configuration > 0) return false;
    if (interface > 0) return false;

    memcpy(descriptor, &c1i1d, sizeof(c1i1d));

    return true;
}

bool hal5_usb_device_get_endpoint_descriptor_ex(
        uint8_t configuration,
        uint8_t interface,
        uint8_t endpoint,
        void* descriptor)
{
    if (configuration > 0) return false;
    if (interface > 0) return false;
    if (endpoint > 0) return false;

    memcpy(descriptor, &c1i1e1d, sizeof(c1i1e1d));

    return true;
}

bool hal5_usb_device_get_string_descriptor_ex(
        uint8_t index,
        uint8_t lang_id,
        void* descriptor)
{
    if (index > (sizeof(sds) / sizeof(sds[0]))) return false;

    memcpy(descriptor, &sds[index], sizeof(sds[index]));

    return true;
}

bool hal5_usb_device_is_device_self_powered_ex() 
{ 
    return true; 
}

bool hal5_usb_device_clear_endpoint_halt_ex(
        uint8_t endpoint,
        bool dir_in)
{
    return false;
}

bool hal5_usb_device_set_endpoint_halt_ex(
        uint8_t endpoint,
        bool dir_in)
{
    return false;
}

bool hal5_usb_device_is_endpoint_halt_set_ex(
        uint8_t endpoint, 
        bool dir_in,
        bool* is_set) 
{
    return false;
}

bool hal5_usb_device_clear_device_remote_wakeup_ex()
{
    return false;
}

bool hal5_usb_device_set_device_remote_wakeup_ex()
{
    return false;
}

bool hal5_usb_device_is_device_remote_wakeup_set_ex()
{
    return false;
}

bool hal5_usb_device_set_test_mode_ex()
{
    return false;
}

bool hal5_usb_device_is_test_mode_set_ex()
{
    return false;
}

uint8_t hal5_usb_device_get_current_configuration_value_ex()
{
    return 0;
}

void hal5_usb_device_set_configuration_ex(
        uint8_t configuration_value)
{
}

void hal5_usb_device_out_transaction_completed_ex(
        hal5_usb_transaction_t* trx)
{
}

void hal5_usb_device_in_transaction_completed_ex(
        hal5_usb_transaction_t* trx)
{
}
