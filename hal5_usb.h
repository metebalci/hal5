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

#define __HAL5_DEBUG_USB__

#define MCU_USB_STRING_DESCRIPTOR_MAX_UTF16_LENGTH 64

// usb channel/endpoint registers
// it is easier to access like this
#define USB_CHEP  ((uint32_t*) USB_DRD_BASE)

// usb sram
#define USB_SRAM  ((uint8_t*) USB_DRD_PMAADDR)

// usb buffer descriptors
// this is inside usb sram, starting from offset 0
// each descriptor is 2 x 4 bytes
#define USB_BD    (USB_DRD_PMA_BUFF)

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
} hal5_usb_device_descriptor_t;

typedef __PACKED_STRUCT
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint16_t bcdUSB;
    uint8_t bDeviceClass;
    uint8_t bDeviceSubClass;
    uint8_t bDeviceProtocol;
    uint8_t bMaxPacketSize0;
    uint8_t bNumConfigurations;
    uint8_t bReserved;
} hal5_usb_device_qualifier_descriptor_t;

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
} hal5_usb_configuration_descriptor_t;

typedef __PACKED_STRUCT
{
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndPoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} hal5_usb_interface_descriptor_t;

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
    // a randomly set maximum length of string
    // increase this if you need a longer string 
    // this is not used as length when sending, bLength is used
    uint8_t bString[MCU_USB_STRING_DESCRIPTOR_MAX_UTF16_LENGTH];
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
    transfer_type_bulk=0b00,
    transfer_type_control=0b01,
    transfer_type_iso=0b10,
    transfer_type_interrupt=0b11
} usb_transfer_type_t;

typedef struct
{
    uint8_t     idn;
    uint8_t     ea;

    uint32_t    istr;
    uint32_t    chep;

    uint8_t     rx_data[1024] __ALIGNED(4);
    uint32_t    rx_data_size;
    uint32_t    rx_data_len;

    uint8_t     tx_data[1024] __ALIGNED(4);
    uint32_t    tx_data_size;
    uint32_t    tx_data_len;

    // rx_data parsed as device_request
    // used by ep0
    hal5_usb_device_request_t* device_request;

} hal5_usb_transaction_t;

void hal5_usb_configure();

void hal5_usb_initialize_buffer_descriptors(
        bool dir_in[], 
        uint16_t size[]);

void hal5_usb_prepare_endpoint(
        hal5_usb_transaction_t* trx,
        usb_ep_status_t rx_status,
        usb_ep_status_t tx_status);

void hal5_usb_copy_to_endpoint(hal5_usb_transaction_t*);
void hal5_usb_copy_from_endpoint(hal5_usb_transaction_t*);

#ifdef __cplusplus
}
#endif

#endif
