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

#ifndef __HAL5_USB_DEVICE_H__
#define __HAL5_USB_DEVICE_H__

#include <stdbool.h>
#include <stdint.h>

#include "hal5_usb.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const hal5_usb_device_descriptor_t* const hal5_usb_device_descriptor __WEAK;
extern const uint32_t hal5_usb_number_of_string_descriptors __WEAK;
extern const hal5_usb_string_descriptor_t* const hal5_usb_string_descriptors[] __WEAK;
extern const bool hal5_usb_product_string_append_version __WEAK;

typedef enum 
{
    usb_device_state_default,
    usb_device_state_address,
    usb_device_state_configured,
} hal5_usb_device_state_t;

void hal5_usb_device_connect(void);
void hal5_usb_device_disconnect(void);

hal5_usb_device_state_t hal5_usb_device_get_state();

// these are called from endpoint 0 implementation
// do not call these if you do not know what you are doing
void hal5_usb_device_set_address(uint8_t device_address);

// get configuration value but according to state rules
uint8_t hal5_usb_device_get_configuration_value();
// set configuration value but also change state if needed
bool hal5_usb_device_set_configuration_value(uint8_t configuration_value);

// endpoint 0 - enumeration support
// these are implemented by hal5_usb_device_ep0.c
void hal5_usb_device_setup_transaction_completed_ep0(
        hal5_usb_endpoint_t *ep);

void hal5_usb_device_out_stage_completed_ep0(
        hal5_usb_endpoint_t *ep);

void hal5_usb_device_in_stage_completed_ep0(
        hal5_usb_endpoint_t *ep);

usb_standard_request_t hal5_usb_device_ep0_get_standard_request();

// _ex functions
// must be provided by device implementations
// see example_usb_device.c as an example

// between 0 and 99
uint8_t hal5_usb_device_version_major_ex();
// between 9 and 99
uint8_t hal5_usb_device_version_minor_ex();

// Self Powered cannot be changed by Set/Clear Feature
bool hal5_usb_device_is_device_self_powered_ex();

// ENDPOINT_HALT
bool hal5_usb_device_clear_endpoint_halt_ex(
        uint8_t endpoint, 
        bool dir_in);
bool hal5_usb_device_set_endpoint_halt_ex(
        uint8_t endpoint, 
        bool dir_in);
bool hal5_usb_device_is_endpoint_halt_set_ex(
        uint8_t endpoint,
        bool dir_in,
        bool* is_set);

// DEVICE_REMOTE_WAKEUP
bool hal5_usb_device_clear_device_remote_wakeup_ex();
bool hal5_usb_device_set_device_remote_wakeup_ex();
bool hal5_usb_device_is_device_remote_wakeup_set_ex();

// TEST_MODE
// TEST_MODE cannot be cleared by Clear Feature
bool hal5_usb_device_set_test_mode_ex();
bool hal5_usb_device_is_test_mode_set_ex();

// synch frame if supported
// return false if not supported
bool hal5_usb_device_get_synch_frame_ex(
        uint8_t endpoint,
        bool dir_in,
        uint16_t* frame_number);

void hal5_usb_device_set_configuration_ex(
        uint8_t configuration_value);

bool hal5_usb_device_get_interface_ex(
        uint8_t interface,
        uint8_t* alternate_setting);

bool hal5_usb_device_set_interface_ex(
        uint8_t interface,
        uint8_t alternate_setting);

void hal5_usb_device_out_stage_completed_ex(
        hal5_usb_endpoint_t *ep) __WEAK;

void hal5_usb_device_in_stage_completed_ex(
        hal5_usb_endpoint_t *ep) __WEAK;

#ifdef __cplusplus
}
#endif

#endif
