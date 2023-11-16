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

#include <stm32h5xx.h>

#include "hal5.h"

void hal5_usb_configure()
{
    // USB uses HSI48
    hal5_rcc_enable_hsi48();

    // enable crs to adjust hsi48 synced to USB SOF
    hal5_crs_enable_for_usb();

    // use hsi48_ker_ck as USB kernel clock
    // default is none
    MODIFY_REG(RCC->CCIPR4, RCC_CCIPR4_USBSEL_Msk,
            0b11UL << RCC_CCIPR4_USBSEL_Pos);

    // these enable GPIO port A implicitly
    // PA11 USB_DM D- (USB_FS_N)
    // PA12 USB_DP D+ (USB_FS_P)
    hal5_gpio_configure_as_af(
            PA11,
            af_pp_floating,
            high_speed,
            AF10);

    hal5_gpio_configure_as_af(
            PA12,
            af_pp_floating,
            high_speed,
            AF10);

    // enable USB IRQ
    NVIC_SetPriority(USB_DRD_FS_IRQn, 6);
    NVIC_EnableIRQ(USB_DRD_FS_IRQn);

    // enable PWR for USB
    hal5_pwr_enable_usb33();
    printf("USB3V3 is valid.\n");

    // enable USB register macrocell clock
    hal5_rcc_enable_usb();

    // power up USB (exit USB power down)
    // after this the reset state is held (USB_CNTR_USBRST is 1)
    CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_PDWN);

    // wait for USB transciever to stabilize
    // wait for the duration of tSTARTUP
    // but it is already 1us, so just waiting 1ms here
    hal5_wait(1);

    // at this point
    // usb is powered and clocked
    // reset is not released
    // pull-up is not enabled

    printf("USB configured.\n");
}


static uint32_t hal5_usb_find_chep_status_toggle(
        uint32_t current, 
        usb_ep_status_t target)
{
    switch (target)
    {
        case ep_status_disabled: 
            // keep as it is, it will toggle to reset
            break;         
        case ep_status_stall:
            // reset first (use as it is), inverse second (use inverse)
            current = (current & 0b10) | (~current & 0b01);
            break;
            break;
        case ep_status_nak:
            // set first (use inverse), reset second (use as it is)
            current = (~current & 0b10) | (current & 0b01);
            break;
        case ep_status_valid: 
            // use inverse, so it will toggle to set
            current = ~current; 
            break;
    }
    return (current & 0b11);
}

void hal5_usb_prepare_endpoint(
        hal5_usb_transaction_t* trx,
        usb_ep_status_t rx_status,
        usb_ep_status_t tx_status)
{
    // only rx or tx can be enabled
    // endpoint is single direction only
    assert ((rx_status == ep_status_disabled) || 
            (tx_status == ep_status_disabled));

    // writing invariant values 1 and 0 to vtrx, vttx and dtogrx, dtogtx 
    // writing invariant values 00 to stattx

    // making statrx 11 by writing inverse of statrx
    // i.e. if it is 10, writing 01 makes it 11
    // making stattx 00 by writing itself
    // i.e. if it is 10, writing 10 makes it 00
    
    const uint32_t chep = trx->chep;
    
    // ATTENTION
    // DO NOT ENABLE RX AND TX AT THE SAME TIME
    // I THINK IT IS NOT WORKING IF BOTH IS ENABLED AT THE SAME TIME
   
    const uint32_t strx = hal5_usb_find_chep_status_toggle(
            ((chep & USB_CHEP_RX_STRX_Msk) >> USB_CHEP_RX_STRX_Pos),
            rx_status);

    const uint32_t sttx = hal5_usb_find_chep_status_toggle(
            ((chep & USB_CHEP_TX_STTX_Msk) >> USB_CHEP_TX_STTX_Pos),
            tx_status);

    //printf("chep: 0x%08lX\n", chep);

    // when device address is assigned
    // CHEP[EA] should contain device address as well
    const uint32_t newchep = (
            0xFFFF0000 | 
            (strx << USB_CHEP_RX_STRX_Pos) | 
            // 0b01=control
            (0b01 << USB_CHEP_UTYPE_Pos) | 
            (sttx << USB_CHEP_TX_STTX_Pos) | 
            trx->ea);

    //printf("chep: 0x%08lX\n", newchep);
    //printf("chep: 0x%08lX\n", apply_chep(chep, newchep));

    USB_CHEP[trx->ea] = newchep;
}

// ATTENTION
// copy from and copy to USB SRAM is word aligned
// do not use memcpy

// this function assumes the buffer descriptor rx_addr is 4-bytes aligned
// this is ensured when buffer descriptors are initialized
// so if tx_data_len is not a multiple of 4 bytes
// it actually copies more (up to allocated) but sets tx_count exact
void hal5_usb_copy_to_endpoint(hal5_usb_transaction_t* trx)
{
    if (trx->tx_data_len > 0)
    {
        const uint32_t* data32 = (uint32_t*) trx->tx_data;

        const uint32_t len_rounded_up = 
            (trx->tx_data_len + (trx->tx_data_len & 0x03)) >> 2;

        uint32_t* addr = 
            (uint32_t*) (USB_SRAM + (USB_BD[trx->idn].TXBD & 0xFFFF));

        // copy word aligned parts
        for (uint32_t i = 0; i < len_rounded_up; i++)
        {
            addr[i] = data32[i];
        }
    }

    // update count in buffer descriptor
    USB_BD[trx->idn].TXBD = 
        (USB_BD[trx->idn].TXBD & 0xFC00FFFF) | (trx->tx_data_len << 16);
}

// this function writes more to trx->rx_data 
//  if rx_count is not a 4 bytes multiple
// but trx->rx_data_len is set as rx_count
void hal5_usb_copy_from_endpoint(hal5_usb_transaction_t* trx)
{
    const uint32_t* buffer32 = 
        (uint32_t*) (USB_SRAM + (USB_BD[trx->idn].RXBD & 0xFFFF));

    const uint32_t rx_count = (USB_BD[trx->idn].RXBD & 0x0CFF0000) >> 16;

    const uint32_t rx_count_rounded_up = rx_count + (rx_count & 0x3);

    uint32_t* data32 = (uint32_t*) trx->rx_data;

    for (uint32_t i = 0; i < rx_count_rounded_up; i++)
    {
        data32[i] = buffer32[i];
    }

    trx->rx_data_len = rx_count;
}

static uint32_t hal5_usb_rxbd_pack(
        uint32_t allocated_memory, 
        uint32_t rx_addr_offset)
{
    assert (allocated_memory > 0);
    // max 1024? but actually it is set as 1023?
    assert (allocated_memory <= 1024);
    // has to be even size
    assert (allocated_memory % 2 == 0);
    // rx_addr has to be 4-bytes aligned
    assert (rx_addr_offset % 4 == 0);
    assert (rx_addr_offset <= 0xFFFF);

    if (allocated_memory < 64) 
    {
        // blsize = 0
        return (((allocated_memory / 2) << 26) | rx_addr_offset);
    }
    else
    {
        // blsize = 1
        return (0x80000000 | (((allocated_memory / 32) - 1) << 26) | rx_addr_offset);
    }
}

static uint32_t hal5_usb_txbd_pack(
        uint32_t tx_addr_offset)
{
    // tx_addr has to be 4-bytes aligned
    assert (tx_addr_offset % 4 == 0);
    assert (tx_addr_offset <= 0xFFFF);
    return tx_addr_offset;
}

// initialize endpoint buffer descriptors
// dir_in is true if dir=IN, false if dir=OUT
// size is endpoint buffer size (maxPacketSize in descriptors)
// size for unused endpoints should be 0 !
void hal5_usb_initialize_buffer_descriptors(
        bool dir_in[], 
        uint16_t size[])
{
    // starts from 64
    // first 64 bytes are buffer descriptors
    uint32_t next_addr = 64;
    
    // setup other endpoint buffer descriptors
    for (uint8_t ea = 0; ea < 16; ea++)
    {
        // ensure alignment
        // important, copy to and from functions above depends on this
        // so the actual buffer length might be larger than requested
        // to align it to word boundary
        next_addr = next_addr + (next_addr & 0x3);
        // USB SRAM is 2K
        assert (next_addr < 2048);

        // endpoint 0 special treatment
        // both IN and OUT is set and using the same buffer
        // since IN and OUT is not used at the same time
        if (ea == 0)
        {
            USB_BD[ea].RXBD = hal5_usb_rxbd_pack(size[ea], next_addr);
            USB_BD[ea].TXBD = hal5_usb_txbd_pack(next_addr);
        }
        else
        {
            if (dir_in[ea])
            {
                USB_BD[ea].RXBD = hal5_usb_rxbd_pack(size[ea], next_addr);
            }
            else
            {
                USB_BD[ea].TXBD = hal5_usb_txbd_pack(next_addr);
            }
        }

        next_addr += size[ea];
    }
}
