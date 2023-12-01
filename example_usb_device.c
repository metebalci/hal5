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

uint8_t hal5_usb_device_version_major_ex()
{
    return 12;
}

uint8_t hal5_usb_device_version_minor_ex()
{
    return 34;
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
    // HS device should support test mode
    // FS does not need to ?
    return false;
}

bool hal5_usb_device_is_test_mode_set_ex()
{
    return false;
}

bool hal5_usb_device_get_synch_frame_ex(
        uint8_t endpoint,
        bool dir_in,
        uint16_t* frame_number)
{
    return false;
}

void hal5_usb_device_set_configuration_ex(
        uint8_t configuration_value)
{
}

// return current alternate setting
bool hal5_usb_device_get_interface_ex(
        uint8_t interface,
        uint8_t* alternate_setting)
{
    return false;
}

// set current alternate setting
bool hal5_usb_device_set_interface_ex(
        uint8_t interface,
        uint8_t alternate_setting)
{
    return false;
}


void hal5_usb_device_out_stage_completed_ex(
        hal5_usb_endpoint_t* ep)
{
}

void hal5_usb_device_in_stage_completed_ex(
        hal5_usb_endpoint_t* ep)
{
}
