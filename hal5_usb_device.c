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

#include "hal5_usb.h"
#include "hal5_usb_device.h"

static uint8_t usb_device_address;
static hal5_usb_device_state_t usb_device_state;

void hal5_usb_device_set_address(uint8_t address)
{
    usb_device_address = address;

    USB_DRD_FS->DADDR = 0x80 | usb_device_address;

    usb_device_state = usb_device_state_address;
}

void hal5_usb_device_set_configuration(uint8_t configuration_value)
{
    hal5_usb_device_set_configuration_ex(configuration_value);

    usb_device_state = usb_device_state_configured;
}

hal5_usb_device_state_t hal5_usb_device_get_state()
{
    return usb_device_state;
}

// ea is argument here to have same structure as out_transaction_completed
// but ea is ensured/asserted to be 0 in the interrupt handler
static void hal5_usb_device_setup_transaction_completed(
        hal5_usb_transaction_t* trx)
{
    hal5_usb_copy_from_endpoint(trx);
    hal5_usb_device_setup_transaction_completed_ep0(trx);
}

static void hal5_usb_device_out_transaction_completed(
        hal5_usb_transaction_t* trx)
{
    hal5_usb_copy_from_endpoint(trx);

    if (trx->ea == 0)
    {
        hal5_usb_device_out_transaction_completed_ep0(trx);
    } 
    else
    {
        hal5_usb_device_out_transaction_completed_ex(trx);
    }
}

static void hal5_usb_device_in_transaction_completed(
        hal5_usb_transaction_t* trx)
{
    if (trx->ea == 0)
    {
        hal5_usb_device_in_transaction_completed_ep0(trx);
    } 
    else
    {
        hal5_usb_device_in_transaction_completed_ex(trx);
    }
}

void hal5_usb_device_initialize_endpoint_buffers()
{ 
    // first, find buffer sizes and directions of endpoints
    // max packet size of an endpoint is encoded as wMaxPacketSize
    uint16_t size[16] = {0};
    // endpoint directions, if true it is IN, otherwise OUT
    bool dir_in[16] = {false};

    // iterate over descriptors
    // find wMaxPacketSize of each endpoint
    hal5_usb_device_descriptor_t dd;
    hal5_usb_device_get_device_descriptor_ex(&dd);

    // max packet size of endpoint 0 is encoded in device descriptor
    size[0] = dd.bMaxPacketSize0;

    // more than 1 configuration would require a special treatment
    // like changing interfaces/endpoints on the fly
    // and re-configuration buffer descriptors
    // current code does not support this
    assert (dd.bNumConfigurations == 1);

    for (uint8_t i = 0; i < dd.bNumConfigurations; i++)
    {
        hal5_usb_configuration_descriptor_t cd;
        hal5_usb_device_get_configuration_descriptor_ex(i, &cd);

        for (uint32_t j = 0; j < cd.bNumInterfaces; j++)
        {
            hal5_usb_interface_descriptor_t id;
            hal5_usb_device_get_interface_descriptor_ex(i, j, &id);

            for (uint32_t k = 0; k < id.bNumEndPoints; k++)
            {
                hal5_usb_endpoint_descriptor_t ed;
                hal5_usb_device_get_endpoint_descriptor_ex(i, j, k, &ed);

                const uint8_t ea = (ed.bEndpointAddress & 0x0F);

                dir_in[ea] = (ed.bEndpointAddress & 0x80);
                size[ea] = ed.wMaxPacketSize;

            }
        }
    }

    hal5_usb_initialize_buffer_descriptors(dir_in, size);

}

static void hal5_usb_device_reset(void)
{
    //mcu_usb_print_registers();
    
    // device address is set to 0 here
    // it is sent by host with Set Address request
    // and it is assigned to DADDR in bus reset
    usb_device_address = 0;

    printf("USBRST....\n");

    // reset internal state 
    // the following registers are not reset so manually do that
    // this sets RST_DCONM/RESET
    // rx/tx stopped until RST_DCONM/RESET is cleared
    USB_DRD_FS->CNTR    = 0x00000001;
    USB_DRD_FS->ISTR    = 0;
    USB_DRD_FS->BCDR    = 0;
    USB_DRD_FS->DADDR   = 0;

    //mcu_usb_print_registers();

    // not reset by USBRST ?
    /*
    USB_DRD_FS->LPMCSR  = 0;
    */

    // these are reset by USBRST
    // USB_DRD_FS->CHEPnR   = 0;

    // select device mode
    CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_HOST);

    // request bus reset interrupt
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_RESETM);
    // request transfer complete interrupt
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_CTRM);
    // request pma overrun interrupt
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_PMAOVRM);
    // request suspend and wake-up interrupts
    //SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_SUSPM);
    //SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_WKUPM);
    // request error interrupt
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_ERRM);

    // enable (device) function (EF), address is 0
    USB_DRD_FS->DADDR = USB_DADDR_EF;

    // release reset
    // no tx/rx but usb system is ready after this
    // it can detect bus reset etc. and raise interrupt
    CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_USBRST);
}

static void hal5_usb_device_transaction_error(
        hal5_usb_transaction_t* trx)
{
    printf("usb_transaction_error\n");
    const uint32_t statrx = (trx->chep & USB_CHEP_RX_STRX_Msk) >> USB_CHEP_RX_STRX_Pos;
    const uint32_t stattx = (trx->chep & USB_CHEP_TX_STTX_Msk) >> USB_CHEP_TX_STTX_Pos;

    const char* msg[] = {"DISABLED", "STALL", "NAK", "VALID"};

    printf("--- USB Communication Error ---\n");

    printf("RX %s\n", msg[statrx]);
    printf("TX %s\n", msg[stattx]);

    assert (false);
}

static void hal5_usb_device_bus_error(void)
{
    printf("usb_bus_error\n");
}

static void hal5_usb_device_bus_reset(void)
{
    //printf("usb_bus_reset\n");  

    // USB BUS RESET does not happen only once before setup
    // it can happen anytime and it does not reset the state
    // so do not reset the internal state with bus reset
    
    usb_device_state = usb_device_state_default;
    
    // enable control endpoint 0
    //hal5_usb_prepare_endpoint();
}

static void hal5_usb_device_suspend(void)
{
    printf("usb_suspend\n");
}

static void hal5_usb_device_wakeup(void)
{
    printf("usb_wakeup\n");
}

static void hal5_usb_device_buffer_overflow(void)
{
    printf("usb_buffer_overflow\n");
}

void USB_DRD_FS_IRQHandler(void)
{
    const uint32_t istr = USB_DRD_FS->ISTR;

    if (istr & USB_ISTR_RESET_Msk) 
    {
        // bus reset detected
        // D+ and D- both pulled down (by the host) for > 10ms

        // avoid read-modify-write of ISTR
        // clear RESET (called RST_DCON in reference manual)
        // suspend condition check is enabled immediately after any USB reset
        // so clear it as well
        USB_DRD_FS->ISTR = ~((1 << USB_ISTR_SUSP_Pos) | (1 << USB_ISTR_RESET_Pos));
        hal5_usb_device_bus_reset();
    } 
    else if (istr & USB_ISTR_CTR) 
    {
        // transfer completed (ACKed, NAKed or STALLed)
        // this interrupt is called after USB transaction is finished
        
        hal5_usb_transaction_t trx;

        // ISTR CTR bit is read-only
        // no need to clear any bit in ISTR

        trx.idn    = (istr & USB_ISTR_IDN_Msk) & 0xF;
        trx.chep   = USB_CHEP[trx.idn];
        trx.ea     = (trx.chep & 0xF);

        //printf("CTR (%lu, 0x%08lX)\n",  current_idn, istr);
        //printf("CHEP[%lu] 0x%08lX\n",   current_idn, istr);

        const bool vtrx = (trx.chep & USB_CHEP_VTRX_Msk);
        const bool vttx = (trx.chep & USB_CHEP_VTTX_Msk);

        if (vtrx) 
        {
            // SETUP or OUT transaction is completed, from host to device
            // this happens when SETUP or OUT is ACKed by the device
            const bool setup = (trx.chep & USB_CHEP_SETUP_Msk);
            if (setup) 
            {
                assert (trx.ea == 0);
                // SETUP transaction
                hal5_usb_device_setup_transaction_completed(&trx);
            } 
            else
            {
                hal5_usb_device_out_transaction_completed(&trx);
            }
        }
        else if (vttx) 
        {
            // IN transaction is completed, from device to host
            // this happens when IN is ACKed by host
            //
            hal5_usb_device_in_transaction_completed(&trx);
        }
        else
        {
            // SETUP, OUT or IN transaction is not completed, not ACKed
            // so either a NAK or STALL received
            hal5_usb_device_transaction_error(&trx);
        }
    } 
    else if (istr & USB_ISTR_PMAOVR) 
    {
        // PMA overrun/underrun detected
        
        // avoid read-modify-write of PMAOVR
        // clear PMAOVR
        USB_DRD_FS->ISTR = ~(1 << USB_ISTR_PMAOVR_Pos);
        hal5_usb_device_buffer_overflow();
    } 
    else if (istr & USB_ISTR_ERR) 
    {
        // these errors can usually be ignored, 
        //  because they will be handled by the hardware (retransmission etc)
        // these can be counted and reported as a measure of transmission quality
        // ideally none of these should happen
        // NANS - No answer - timeout waiting for a response
        // CRC - CRC error - token or data CRC was wrong
        // BST - Bit stuffing error
        // FVIO - framing format violation
        
        // avoid read-modify-write of ISTR
        // clear ERR
        USB_DRD_FS->ISTR = ~(1 << USB_ISTR_ERR_Pos);
        hal5_usb_device_bus_error();
    } 
    else if (istr & USB_ISTR_WKUP) 
    {
        // wake-up signalling detected during suspend ?
        
        // avoid read-modify-write of ISTR
        // clear WKUP
        USB_DRD_FS->ISTR = ~(1 << USB_ISTR_WKUP_Pos);
        hal5_usb_device_wakeup();
    } 
    else if (istr & USB_ISTR_SUSP) 
    {
        // suspend detected
        // no activity (no SOF) for >3ms
        // SUSP flag is still set for reset as well
        // so check SUSP flag after checking RESET

        // avoid read-modify-write of ISTR
        // clear SUSP
        USB_DRD_FS->ISTR = ~(1 << USB_ISTR_SUSP_Pos);
        // also set SUSPEN enabled, 
        //  otherwise SUSP interrupt is continuously called 
        SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_SUSPEN);
        hal5_usb_device_suspend();
    } 
    else 
    {
        printf("UNKNOWN INTERRUPT: ISTR: 0x%08lX\n", istr);
        assert (false);
    }
}

void hal5_usb_device_configure()
{
    hal5_usb_device_initialize_endpoint_buffers();

    printf("USB configured.\n");
}

void hal5_usb_device_connect(void) 
{
    printf("USB connect: pulling-up D+ ...\n");

    hal5_usb_device_reset();

    // enable pull-up
    // effectively connects the device
    // host resets the bus first
    // then enumerates
    SET_BIT(USB_DRD_FS->BCDR, USB_BCDR_DPPU);
}

void hal5_usb_device_disconnect(void)
{
    // disable pull-up
    // effectively disconnects the device
    CLEAR_BIT(USB_DRD_FS->BCDR, USB_BCDR_DPPU);
    
    // hold reset
    CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_USBRST);

    printf("USB disconnect: pull-up removed from D+\n");
}
