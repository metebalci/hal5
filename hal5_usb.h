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

typedef union
{
    __IO uint32_t v;
    struct {
        __IO unsigned int idn:4;
        __IO unsigned int dir_out:1;
        __IO unsigned int _reserved1:2;
        __IO unsigned int l1req:1;
        __IO unsigned int esof:1;
        __IO unsigned int sof:1;
        __IO unsigned int rst_dcon:1;
        __IO unsigned int susp:1;
        __IO unsigned int wkup:1;
        __IO unsigned int err:1;
        __IO unsigned int pma_ovr:1;
        __IO unsigned int ctr:1;
        __IO unsigned int thr512:1;
        __IO unsigned int ddisc:1;
        __IO unsigned int _reserved2:11;
        __IO unsigned int dcon_stat:1;
        __IO unsigned int ls_dcon:1;
        __IO unsigned int _reserved3:1;
    };
} hal5_usb_istr_t;

typedef struct
{
    // endpoint number
    uint8_t         endp;
    // cached copy of current ISTR
    hal5_usb_istr_t istr[1];
    // true if endpoint direction is IN
    // control endpoints are bidirectional
    bool            dir_in;
    // endpoint transfer type
    usb_ep_utype_t  utype;
    // max packet size
    uint16_t        mps;
    // temporary copy of data in main memory
    // it simplifies buffer operations
    uint32_t*       packet32 __ALIGNED(4);

    bool last_out;
    bool current_out;

    // chep is cached chep_reg when trx completed
    hal5_usb_chep_t  chep[1];
    // chep_reg to write back
    // write to chep_reg is done only once
    hal5_usb_chep_t  chep2sync[1];
    // actual chep_reg access
    hal5_usb_chep_t* chep_reg;    

    // buffer descriptor, only if endpoint supports SETUP/OUT
    hal5_usb_bd_t*  rxbd;
    // buffer descriptor, only if endpoint supports IN
    hal5_usb_bd_t*  txbd;

    // rx buffer addr in USB SRAM
    void*           rxaddr;
    uint32_t*       rxaddr32;
    // the next ep rx status
    usb_ep_status_t rx_status;

    // rx buffer addr in main memory
    uint8_t*        rx_data __ALIGNED(4);
    uint32_t*       rx_data32;
    // actual amount received to rx_data
    size_t          rx_received;

    // tx buffer addr in USB SRAM
    void*           txaddr;
    uint32_t*       txaddr32;
    // flag that controls if ZLP is sent before or no
    bool            tx_zlp_sent;
    // true if tx_expected contains a valid data
    // in other words, if there is a data amount expected by the host
    bool            tx_expected_valid;
    // data amount expected by the host
    size_t          tx_expected;
    // data amount actually sent
    size_t          tx_sent;
    // actual amount that is going to be sent
    // this might be different than tx_data_size due to tx_expected
    size_t          tx_sent_limit;
    // the next ep tx status
    usb_ep_status_t tx_status;

    // tx buffer addr in main memory
    uint8_t*        tx_data __ALIGNED(4);
    uint32_t*       tx_data32;
    // the amount of data in tx buffer 
    size_t          tx_data_size;

    // rx_data cast as device_request for ease of use
    hal5_usb_device_request_t* device_request;

} hal5_usb_endpoint_t;

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
        const size_t data_size,
        const bool expected_valid,
        const size_t expected);

void hal5_usb_ep_prepare_for_out(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t tx_status);

#ifdef __cplusplus
}
#endif

#endif
