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

#ifndef __HAL5_USB_H__
#define __HAL5_USB_H__

#include <stdint.h>

#include <stm32h5xx.h>

#define FEATURE_SELECTOR_ENDPOINT_HALT          (0)
#define FEATURE_SELECTOR_DEVICE_REMOTE_WAKEUP   (1) 
#define FEATURE_SELECTOR_TEST_MODE              (2) 

// usb sram
#define USB_SRAM  ((uint8_t*) USB_DRD_PMAADDR)

#ifdef __cplusplus
extern "C" {
#endif

typedef __PACKED_STRUCT
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} hal5_usb_device_request_t;

typedef __PACKED_STRUCT 
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bEndpointAddress;
    uint8_t bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t bInterval;
} hal5_usb_endpoint_descriptor_t;

typedef __PACKED_STRUCT
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
    const hal5_usb_endpoint_descriptor_t* const endpoints[];
} hal5_usb_interface_descriptor_t;

typedef __PACKED_STRUCT
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t wTotalLength;
    uint8_t bNumInterfaces;
    uint8_t bConfigurationValue;
    uint8_t iConfiguration;
    uint8_t bmAttributes;
    uint8_t bMaxPower;
    const hal5_usb_interface_descriptor_t* const interfaces[];
} hal5_usb_configuration_descriptor_t;

typedef __PACKED_STRUCT
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
    const hal5_usb_configuration_descriptor_t* const configurations[];
} hal5_usb_device_descriptor_t;

typedef __PACKED_STRUCT
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    // a randomly set maximum length of string
    // increase this if you need a longer string 
    // this is not used as length when sending, bLength is used
    uint8_t bString[];
} hal5_usb_string_descriptor_t;

typedef enum
{
    ep_status_disabled=0b00,
    ep_status_stall=0b01,
    ep_status_nak=0b10,
    ep_status_valid=0b11,
} usb_ep_status_t;

typedef enum 
{
    ep_utype_bulk=0b00,
    ep_utype_control=0b01,
    ep_utype_iso=0b10,
    ep_utype_interrupt=0b11
} usb_ep_utype_t;

// the definitions here are sensitive
// only union and struct/__PACKED_STRUCT combination is working as intended
// not sure why
typedef union
{
    __IO uint32_t v;
    struct {
        __IO unsigned int addr:16;
        __IO unsigned int count:10;
        __IO unsigned int num_block:5;
        __IO unsigned int blsize:1;
    };
} hal5_usb_bd_t;

typedef union
{
    __IO uint32_t v;
    struct {
        __IO unsigned int ea:4;
        __IO unsigned int stattx:2;
        __IO unsigned int dtogtx:1;
        __IO unsigned int vttx:1;
        __IO unsigned int epkind:1;
        __IO unsigned int utype:2;
        __IO unsigned int setup:1;
        __IO unsigned int statrx:2;
        __IO unsigned int dtogrx:1;
        __IO unsigned int vtrx:1;

        __IO unsigned int devaddr:7;
        __IO unsigned int nak:1;
        __IO unsigned int ls_ep:1;
        __IO unsigned int err_tx:1;
        __IO unsigned int err_rx:1;
        __IO unsigned int three_err_tx:2;
        __IO unsigned int three_err_rx:2;
        __IO unsigned int _reserved:1;
    };
} hal5_usb_chep_t;

typedef struct
{
    uint8_t         endp;
    bool            dir_in;
    usb_ep_utype_t  utype;
    uint16_t        mps;
    // temporary copy of data in main memory
    uint32_t*       packet32 __ALIGNED(4);

    // chep is cached chep_reg when trx completed
    hal5_usb_chep_t  chep[1];
    hal5_usb_chep_t  chep2sync[1];
    hal5_usb_chep_t* chep_reg;    

    hal5_usb_bd_t*  rxbd;
    hal5_usb_bd_t*  txbd;

    void*           rxaddr;
    uint32_t*       rxaddr32;
    size_t          rx_received;
    usb_ep_status_t rx_status;

    uint8_t*        rx_data __ALIGNED(4);
    uint32_t*       rx_data32;
    size_t          rx_data_size;

    void*           txaddr;
    uint32_t*       txaddr32;
    size_t          tx_sent;
    bool            tx_expected_valid;
    size_t          tx_expected;
    usb_ep_status_t tx_status;

    uint8_t*        tx_data __ALIGNED(4);
    uint32_t*       tx_data32;
    size_t          tx_data_size;

    hal5_usb_device_request_t* device_request;

} hal5_usb_endpoint_t;

void hal5_usb_configure(void);

size_t hal5_usb_device_copy_to_endpoint(
        hal5_usb_endpoint_t* ep);

size_t hal5_usb_device_copy_from_endpoint(
        hal5_usb_endpoint_t* ep);

// pass ed=NULL for endpoint 0
// then it automatically reads the max packet size 
// from hal5_usb_device_descriptor
// and assumes it is a control endpoint
hal5_usb_endpoint_t* hal5_usb_ep_create(
        const hal5_usb_endpoint_descriptor_t* ed,
        const uint32_t next_bd_addr);

void hal5_usb_ep_free(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_dump_status(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_sync_from_reg(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_sync_to_reg(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_clear_data(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_clear_vtrx(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_clear_vttx(
        hal5_usb_endpoint_t* ep);

void hal5_usb_ep_set_status(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t rx_status,
        usb_ep_status_t tx_status);

void hal5_usb_ep_prepare_for_in(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t rx_status,
        const void* data,
        const size_t len,
        const bool tx_expected_valid,
        const size_t tx_expected);

void hal5_usb_ep_prepare_for_out(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t tx_status);

#ifdef __cplusplus
}
#endif

#endif
