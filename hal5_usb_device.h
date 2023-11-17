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

#define FEATURE_SELECTOR_ENDPOINT_HALT          (0)
#define FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP   (1) 
#define FEATURE_SELECTOR_TEST_MODE              (2) 

#ifdef __cplusplus
extern "C" {
#endif

typedef enum 
{
    usb_device_state_attached,
    usb_device_state_powered,
    usb_device_state_default,
    usb_device_state_address,
    usb_device_state_configured,
    usb_device_state_suspended
} hal5_usb_device_state_t;

void hal5_usb_device_configure(void);
void hal5_usb_device_connect(void);
void hal5_usb_device_disconnect(void);

hal5_usb_device_state_t hal5_usb_device_get_state();

// these are called from endpoint 0 implementation
// do not call these if you do not know what you are doing
void hal5_usb_device_set_address(uint8_t device_address);
bool hal5_usb_device_set_configuration(uint8_t configuration_value);

// endpoint 0 - enumeration support
// these are implemented by hal5_usb_device_ep0.c
void hal5_usb_device_setup_transaction_completed_ep0(
        hal5_usb_transaction_t *trx);

void hal5_usb_device_out_transaction_completed_ep0(
        hal5_usb_transaction_t *trx);

void hal5_usb_device_in_transaction_completed_ep0(
        hal5_usb_transaction_t *trx);

// _ex functions
// must be provided by device implementations
// see hal5_usb_device_example.c as an example

void hal5_usb_device_get_device_descriptor_ex(
        void* descriptor) __WEAK;

bool hal5_usb_device_get_configuration_descriptor_ex(
        uint8_t configuration,
        void* descriptor) __WEAK;

bool hal5_usb_device_get_interface_descriptor_ex(
        uint8_t configuration,
        uint8_t interface,
        void* descriptor) __WEAK;

bool hal5_usb_device_get_endpoint_descriptor_ex(
        uint8_t configuration,
        uint8_t interface,
        uint8_t endpoint,
        void* descriptor) __WEAK;

bool hal5_usb_device_get_string_descriptor_ex(
        uint8_t index,
        uint8_t lang_id,
        void* descriptor) __WEAK;

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

// return 0 if not configured
uint8_t hal5_usb_device_get_current_configuration_value_ex() __WEAK;

bool hal5_usb_device_set_configuration_ex(
        uint8_t configuration_value) __WEAK;

void hal5_usb_device_out_transaction_completed_ex(
        hal5_usb_transaction_t *trx) __WEAK;

void hal5_usb_device_in_transaction_completed_ex(
        hal5_usb_transaction_t *trx) __WEAK;

#ifdef __cplusplus
}
#endif

#endif
