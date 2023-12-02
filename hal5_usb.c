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
#include <stdlib.h>
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
    
    CONSOLE("USB configured.\n");
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

uint32_t apply_to_chep(uint32_t old, uint32_t new)
{
    uint32_t rc_w0  = 0b01111110100000001000000010000000;
    uint32_t t      = 0b00000000000000000111000001110000;
    uint32_t r      = 0b00000000000000000000100000000000;
    uint32_t rw     = 0b00000001011111110000011100001111;

    uint32_t cleared = old & ~(t & new);
    uint32_t toggled = old ^ (t & new);
    uint32_t read    = r & old;
    uint32_t written = rw & new;

    return (cleared | toggled | read | written);
}

void hal5_usb_ep_clear_data(
        hal5_usb_endpoint_t* ep)
{
    ep->rx_received = 0;

    ep->tx_zlp_sent         = false;
    ep->tx_expected_valid   = 0;
    ep->tx_expected         = 0;

    ep->tx_sent         = 0;
    ep->tx_data_size    = 0;

}

void hal5_usb_ep_clear_vtrx(
        hal5_usb_endpoint_t* ep)
{
    ep->chep2sync->vtrx = 0;
}

void hal5_usb_ep_clear_vttx(
        hal5_usb_endpoint_t* ep)
{
    ep->chep2sync->vttx = 0;
}

void hal5_usb_ep_set_status(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t rx_status,
        usb_ep_status_t tx_status)
{
    ep->chep2sync->statrx = hal5_usb_find_chep_status_toggle(
            ep->chep->statrx,
            rx_status);

    ep->rx_status = rx_status;

    ep->chep2sync->stattx = hal5_usb_find_chep_status_toggle(
            ep->chep->stattx,
            tx_status);

    ep->tx_status = tx_status;
}

static void dump_status(
        usb_ep_status_t status)
{
    switch (status)
    {
        case ep_status_valid: CONSOLE("V"); break;
        case ep_status_stall: CONSOLE("S"); break;
        case ep_status_nak: CONSOLE("N"); break;
        case ep_status_disabled: CONSOLE("X"); break;
    }
}

void hal5_usb_ep_dump_status(
        hal5_usb_endpoint_t* ep)
{
    dump_status(ep->rx_status);
    dump_status(ep->tx_status);
    CONSOLE("\n");
}

void hal5_usb_ep_sync_from_reg(
        hal5_usb_endpoint_t* ep)
{
    ep->chep->v = ep->chep_reg->v;
    ep->chep2sync->v = ep->chep->v;

    // below are the defaults

    // vtrx and vttx can only be cleared with 0
    // they can be directly set, e.g. ep->chep->vtrx = 0
    
    // dtogrx, statrx, dtogtx, stattx can only be toggled with 1
    // they should be set with ep_set_... methods

    // the fields below are modified from original
    // clear-only does not clear
    // toggle does not toggle
    ep->chep2sync->three_err_rx = 0b11; // clear only
    ep->chep2sync->three_err_tx = 0b11; // clear only
    ep->chep2sync->err_rx = 0b1;        // clear only
    ep->chep2sync->err_tx = 0b1;        // clear only
                                        // ls_ep is rw
    ep->chep2sync->nak = 0b1;           // clear only
                                        // devaddr is rw
    ep->chep2sync->vtrx = 0b1;          // clear only
    ep->chep2sync->dtogrx = 0b0;        // toggle
    ep->chep2sync->statrx = 0b00;       // toggle
                                        // setup is read only
                                        // utype is rw
                                        // epkind is rw
    ep->chep2sync->vttx = 0b1;          // clear only
    ep->chep2sync->dtogtx = 0b0;        // toggle
    ep->chep2sync->stattx = 0b00;       // toggle
                                        // ea is rw
    
    hal5_usb_ep_set_status(
            ep, 
            ep_status_disabled,
            ep_status_disabled);
}

void hal5_usb_ep_sync_to_reg(
        hal5_usb_endpoint_t* ep)
{
    ep->chep_reg->v = ep->chep2sync->v;
}

hal5_usb_endpoint_t* hal5_usb_ep_create(
        const hal5_usb_endpoint_descriptor_t* ed,
        const uint32_t next_bd_addr)
{
    // first 64 bytes are buffer descriptors
    assert (next_bd_addr >= 64);
    // USB_SRAM is 2048 bytes
    assert (next_bd_addr <= 2048);
    // USB_SRAM is word wide, so addr must be word aligned
    assert ((next_bd_addr % 4) == 0);

    uint32_t endpoint_address;
    uint16_t max_packet_size;
    usb_ep_utype_t utype;
    if (ed == NULL) 
    {
        endpoint_address = 0;
        max_packet_size = hal5_usb_device_descriptor->bMaxPacketSize0;
        utype = ep_utype_control;
    }
    else
    {
        endpoint_address = ed->bEndpointAddress;
        max_packet_size = ed->wMaxPacketSize;
        switch (ed->bmAttributes & 0x3)
        {
            // control
            case 0b00: utype = ep_utype_control; break;
            // iso
            case 0b01: utype = ep_utype_iso; break; 
            // bulk
            case 0b10: utype = ep_utype_bulk; break;
            // interrupt
            case 0b11: utype = ep_utype_interrupt; break;
            default: assert (false);
        }
    }

    hal5_usb_endpoint_t* ep = (hal5_usb_endpoint_t*) 
        calloc(1, sizeof(hal5_usb_endpoint_t));
    ep->endp = endpoint_address & 0xF;
    ep->dir_in = endpoint_address & 0x80;
    ep->utype = utype;
    ep->mps = max_packet_size;
    ep->packet32 = (uint32_t*) malloc(ep->mps);

    if ((utype == ep_utype_control) || ep->dir_in)
    {
        ep->rx_data = (uint8_t*) malloc(1024);
        ep->rx_data32 = (uint32_t*) ep->rx_data;
    } 
    else
    {
        ep->rx_data = NULL;
    }

    ep->rx_received = 0;

    if ((utype == ep_utype_control) || !ep->dir_in)
    {
        ep->tx_data = (uint8_t*) malloc(1024);
        ep->tx_data32 = (uint32_t*) ep->tx_data;
    }
    else
    {
        ep->tx_data = NULL;
    }

    ep->tx_sent             = 0;
    ep->tx_zlp_sent         = false;

    ep->chep_reg = (hal5_usb_chep_t*) (USB_DRD_BASE + 4*ep->endp);
    ep->chep_reg->ea = ep->endp;
    ep->chep_reg->utype = ep->utype;
    hal5_usb_ep_sync_from_reg(ep);

    // a buffer descriptor for each endpoint has two 32-bit registers (txbd and rxbd)
    // that means for each endpoint there is an 8 byte buffer descriptor entry
    // this also means the buffer descriptor table (for all endpoints) is
    //   8 endpoints x 8 bytes = 64 bytes (the first 64 bytes of USB SRAM)

    // txbd is first
    if ((ep->utype == ep_utype_control) |
        (ep->dir_in))
    {
        ep->txbd = (hal5_usb_bd_t*) (USB_SRAM + 8*ep->endp);
    }
    // then rxbd (the structure is uint32_t, +1 means +4 bytes)
    if ((ep->utype == ep_utype_control) |
        (!ep->dir_in))
    {
        ep->rxbd = ep->txbd + 1;
    }

    // control endpoint is bidirectional, so requires both rxbd and txbd
    // setup rxbd for control and other endpoints with IN direction
    // rxbd count is set by the hardware
    // the size of (allocated) buffer has to be specified
    if (ep->rxbd != NULL)
    {
        const uint32_t allocated_memory = ep->mps;
        assert (allocated_memory > 0);
        assert (allocated_memory <= 1024);
        assert ((allocated_memory % 2) == 0);

        if (allocated_memory < 64)
        {
            // block size 2 bytes
            ep->rxbd->blsize = 0;
            // num_block = 0 is not allowed, condition already asserted above
            ep->rxbd->num_block = allocated_memory / 2;
        }
        else
        {
            // block size 32 bytes
            ep->rxbd->blsize = 1;
            // the last value actually means 1023 bytes (max packet size of USB FS)
            // -1 because num_block=0 means 32 bytes
            ep->rxbd->num_block = (allocated_memory / 32) - 1;
        } 

        ep->rxbd->count = 0;
        ep->rxbd->addr  = next_bd_addr;

        ep->rxaddr = USB_SRAM + ep->rxbd->addr;
        ep->rxaddr32 = (uint32_t*) ep->rxaddr;
    }

    // setup txbd for control and other endpoints with OUT direction
    // txbd count is set before every transaction
    if (ep->txbd != NULL)
    {
        ep->txbd->count = 0;
        ep->txbd->addr  = next_bd_addr;

        ep->txaddr = USB_SRAM + ep->txbd->addr;
        ep->txaddr32 = (uint32_t*) ep->txaddr;
    }

    /*
    CONSOLE("USB_DRD_BASE   = %p\n", (uint32_t*) USB_DRD_BASE);
    CONSOLE("USB_SRAM       = %p\n", USB_SRAM);
    CONSOLE("ep.rxbd        = %p\n", ep->rxbd);
    CONSOLE("ep.txbd        = %p\n", ep->txbd);
    CONSOLE("ep.rxbd.v      = 0x%08lX\n", ep->rxbd->v);
    CONSOLE("ep.txbd.v      = 0x%08lX\n", ep->txbd->v);
    CONSOLE("ep.rxaddr      = %p\n", ep->rxaddr);
    CONSOLE("ep.txaddr      = %p\n", ep->txaddr);
    */
    /*
    CONSOLE("ep.rxbd.blsize     = %u\n", ep->rxbd->blsize);
    CONSOLE("ep.rxbd.num_block  = %u\n", ep->rxbd->num_block);
    CONSOLE("ep.rxbd.addr       = %u\n", ep->rxbd->addr);
    CONSOLE("ep.txbd.addr       = %u\n", ep->txbd->addr);
    */

    return ep;
}

void hal5_usb_ep_free(
        hal5_usb_endpoint_t* ep)
{
    if (ep != NULL)
    {
        free(ep->packet32);
        free(ep->rx_data);
        free(ep->tx_data);
        free(ep);
    }
}

// ATTENTION
// copy from and copy to USB SRAM is word aligned
// so memcpy cannot be used

// the functions below assumes the buffer pointers are word aligned
// this is ensured when buffer descriptors are initialized

// using an extra buffer as big as max packet size (packet32)
// simplifies the operations

size_t hal5_usb_device_copy_to_endpoint(
        hal5_usb_endpoint_t* ep)
{
    size_t tx_count = ep->tx_sent_limit - ep->tx_sent;
   
    // cannot pass max packet size
    if (tx_count > ep->mps)
    {
        tx_count = ep->mps;
    }

    if (tx_count > 0)
    {
        // COPY bytes IN MAIN MEMORY
        memcpy(
                ep->packet32, 
                ep->tx_data + ep->tx_sent,
                tx_count);

        // len in number of words
        uint32_t len32 = tx_count >> 2;

        // if tx_count is not a multiple of word, add one more word to copy
        if (tx_count & 0x03)
        {
            len32++;
        }

        // COPY words TO USB SRAM
        for (size_t i = 0; i < len32; i++)
        {
            ep->txaddr32[i] = ep->packet32[i];
        }
    }

    ep->txbd->count = tx_count;

    return tx_count;
}

size_t hal5_usb_device_copy_from_endpoint(
        hal5_usb_endpoint_t* ep)
{
    const uint32_t rx_count = ep->rxbd->count;

    // len in number of words
    size_t len32 = rx_count >> 2;

    // if rx_count is not a multiple of word, add one more word to copy
    if (rx_count & 0x03)
    {
        len32++;
    }

    // COPY words FROM USB SRAM
    for (size_t i = 0; i < len32; i++)
    {
        ep->packet32[i] = ep->rxaddr32[i];
    }

    // COPY bytes IN MAIN MEMORY
    memcpy(
            ep->rx_data + ep->rx_received,
            ep->packet32,
            rx_count);

    return rx_count;
}

void hal5_usb_ep_prepare_for_in(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t rx_status,
        const void* data,
        const size_t data_size,
        const bool expected_valid,
        const size_t expected)
{
    hal5_usb_ep_clear_data(ep);

    if (data != NULL)
    {
        memcpy(
                ep->tx_data, 
                data,
                data_size);
    }

    ep->tx_data_size    = data_size;
    ep->tx_sent_limit   = data_size;
    ep->tx_sent         = 0;

    if (expected_valid)
    {
        // if expected size is smaller than data_size
        // then tx_sent_limit is expected
        // do not send more than expected
        ep->tx_expected_valid = true;
        ep->tx_expected = expected;

        if (ep->tx_expected < ep->tx_data_size)
        {
            ep->tx_sent_limit = ep->tx_expected;
        }
    }
    else
    {
        ep->tx_expected_valid = false;
    }

    hal5_usb_ep_set_status(
            ep, 
            rx_status, 
            ep_status_valid);
}

void hal5_usb_ep_prepare_for_out(
        hal5_usb_endpoint_t* ep,
        usb_ep_status_t tx_status)
{
    hal5_usb_ep_clear_data(ep);

    hal5_usb_ep_set_status(
            ep, 
            ep_status_valid, 
            tx_status);
}
