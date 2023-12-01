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


// endpoint number, endpoint direction 0=in, 1=out
hal5_usb_endpoint_t* endpoints[8][2] = {NULL};

static hal5_usb_device_state_t usb_device_state;
static uint8_t usb_device_configuration_value = 0;

// USB (visible) DEVICE STATES
// normally there are
// attached, powered, default, address, configured and suspended states
// attached: when the device is connected physically but no Vbus received
// powered: when the device receives Vbus (independent of it is bus or self powered)
// default: after bus reset 
// address: after the device has an address (after set address request)
// configured: after the device is configured (after set configuration request)
// suspended: after bus inactivity
// 
// if a power interrupt happens (but Vbus stays), device goes to powered
// if bus is idle for some time, device goes to suspended
//   this can happen from powered, default, address or configured
// if it is suspended and there is bus activity, it returns back to the previous state
// device can be de-configured by set configuration (0), so it returns back to address
// the diagram does not say but set address says if address 0 is given, it goes back to default
// a bus reset returns it to default

static void recreate_endpoints_for_configuration(
        const hal5_usb_configuration_descriptor_t* cd)
{
    assert (cd != NULL);

    // this is called from Set Configuration
    // so there can be different configurations = different endpoints
    // all existing endpoints other than 0 should be cleared first
    // clear the buffer descriptors
    memset(USB_SRAM+8, 0, 7*8);
    // free/remove endpoint pointers
    for (uint8_t i = 1; i < 8; i++)
    {
        hal5_usb_ep_free(endpoints[i][0]);
        hal5_usb_ep_free(endpoints[i][1]);
        endpoints[i][0] = NULL;
        endpoints[i][1] = NULL;
    }

    // start from where endpoint 0 ends
    uint32_t next_bd_addr = endpoints[0][0]->rxbd->addr;

    // reinitialize new ones according to descriptors
    for (uint8_t ii = 0; ii < cd->bNumInterfaces; ii++)
    {
        const hal5_usb_interface_descriptor_t* id = 
            cd->interfaces[ii];

        for (uint8_t ei = 0; ei < id->bNumEndpoints; ei++)
        {
            const hal5_usb_endpoint_descriptor_t* ed = 
                id->endpoints[ei];

            // word align the addr if needed
            next_bd_addr = next_bd_addr + (next_bd_addr & 0x3);

            hal5_usb_endpoint_t* ep = hal5_usb_ep_create(
                    ed, 
                    next_bd_addr);

            endpoints[ep->endp][ep->dir_in ? 0 : 1] = ep;

            next_bd_addr += ed->wMaxPacketSize;

        }
    }

}

void hal5_usb_device_set_address(uint8_t address)
{
    assert (address <= 127);
    // device address can be zero or non-zero
    // if it is zero, and state is default, it is not an error
    //  device stays in default state
    // if it is zero, and state is address,
    //  device should go to default state
    // if it is non-zero, it should go to address state
    //  even if it is already in address state
    //  and use the new address
    //
    assert ((usb_device_state == usb_device_state_default) ||
            (usb_device_state == usb_device_state_address));

    USB_DRD_FS->DADDR = 0x80 | address;

    if (address != 0)
    {
        usb_device_state = usb_device_state_address;
    }
    else
    {
        usb_device_state = usb_device_state_default;
    }
}

uint8_t hal5_usb_device_get_configuration_value()
{
    assert ((usb_device_state == usb_device_state_address) ||
            (usb_device_state == usb_device_state_configured));

    switch (usb_device_state)
    {
        case usb_device_state_configured:
            assert (usb_device_configuration_value > 0);
            return usb_device_configuration_value;

        case usb_device_state_address:
            return 0;

        default:
            assert (false);
    }
}

static bool hal5_usb_device_try_changing_configuration(uint8_t configuration_value)
{
    assert (configuration_value > 0);

    for (uint8_t i = 0; i < hal5_usb_device_descriptor->bNumConfigurations; i++)
    {

        const hal5_usb_configuration_descriptor_t* cd = 
            (hal5_usb_configuration_descriptor_t*) hal5_usb_device_descriptor->configurations[i];

        if (configuration_value == cd->bConfigurationValue)
        {
            hal5_usb_device_set_configuration_ex(configuration_value);
            usb_device_configuration_value = configuration_value;
            recreate_endpoints_for_configuration(cd);
            return true;
        }
    }
    return false;
}

bool hal5_usb_device_set_configuration_value(uint8_t configuration_value)
{
    assert ((usb_device_state == usb_device_state_address) ||
            (usb_device_state == usb_device_state_configured));

    if (usb_device_state == usb_device_state_configured)
    {
        if (configuration_value != 0)
        {
            // try to change to a new value
            // will succeed if descriptor contains such a value
            return hal5_usb_device_try_changing_configuration(configuration_value);
        }
        else
        {
            // change back to address state
            hal5_usb_device_set_configuration_ex(0);
            usb_device_configuration_value = 0;
            usb_device_state = usb_device_state_address;
            return true;
        }
    }
    else if (usb_device_state == usb_device_state_address)
    {
        if (configuration_value != 0)
        {
            // will succeed if descriptor contains such a value
            if (hal5_usb_device_try_changing_configuration(configuration_value))
            {
                // if succeeds, then it is configured state now
                usb_device_state = usb_device_state_configured;
                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            // if configuration_value is zero
            // stay in address state
            return true;
        }
    }
    else
    {
        assert (false);
    }
}

hal5_usb_device_state_t hal5_usb_device_get_state()
{
    return usb_device_state;
}

static void hal5_usb_device_out_stage_completed(
        hal5_usb_endpoint_t* ep)
{
    if (ep->endp == 0)
    {
        hal5_usb_device_out_stage_completed_ep0(ep);
    } 
    else
    {
        hal5_usb_device_out_stage_completed_ex(ep);
    }
}

static void hal5_usb_device_in_stage_completed(
        hal5_usb_endpoint_t* ep)
{
    if (ep->endp == 0)
    {
        hal5_usb_device_in_stage_completed_ep0(ep);
    } 
    else
    {
        hal5_usb_device_in_stage_completed_ex(ep);
    }
}

static void hal5_usb_device_reset(void)
{
    CONSOLE("usb device reset\n");

    //mcu_usb_print_registers();
    
    // device address is set to 0 here
    // it is sent by host with Set Address request
    // and it is set to DADDR in hal5_usb_device_set_address function above
    usb_device_state = usb_device_state_default;

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
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_SUSPM);
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_WKUPM);
    // request error interrupt
    SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_ERRM);

    // enable (device) function (EF), address is 0
    USB_DRD_FS->DADDR = USB_DADDR_EF;

    // release reset
    // no tx/rx but usb system is ready after this
    // it can detect bus reset etc. and raise interrupt
    CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_USBRST);

}

static void hal5_usb_device_transaction_completed(
        hal5_usb_endpoint_t* ep)
{
    if (ep->chep->vtrx) 
    {
        // reset so the interrupt is not raised again
        hal5_usb_ep_clear_vtrx(ep);

        // if OUT/SETUP received
        // clear EP TX state
        ep->tx_data_size    = 0;
        ep->tx_sent         = 0;

        ep->tx_expected_valid   = false;
        ep->tx_expected         = 0;

        // SETUP or OUT transaction is completed, from host to device
        // this happens when SETUP or OUT is ACKed by the device
        
        // is it SETUP or OUT ?
        if (ep->chep->setup) 
        {
            //CONSOLE("SETUP (%u)\n", ep->rx_received);

            // setup transaction always has 8 bytes of DATA0
            assert (ep->rx_received == 8);
            // there is no need to check if data phase is finished
            // since minimum of max packet size is 8 bytes
            ep->device_request = (hal5_usb_device_request_t*) ep->rx_data;
            hal5_usb_device_setup_transaction_completed_ep0(ep);
        } 
        else
        {
            CONSOLE("OUT (%u, %u, %u)\n", 
                    ep->mps,
                    ep->rxbd->count,
                    ep->rx_received);

            if (ep->rxbd->count < ep->mps)
            {
                // done
                // if a less than packet size data arrives
                // it means data stage is terminated
                hal5_usb_device_out_stage_completed(ep);
            }
            else
            {
                // read more
                hal5_usb_ep_set_status(
                        ep,
                        ep_status_valid,
                        ep_status_stall);
            }
        }
    }
    else if (ep->chep->vttx) 
    {
        // reset so the interrupt is not raised again
        hal5_usb_ep_clear_vttx(ep);

        // if IN received
        // clear EP RX state
        ep->rx_received = 0;

        CONSOLE("IN (%u, %u, %u/%u-", 
                ep->mps,
                ep->txbd->count,
                ep->tx_sent,
                ep->tx_data_size);

        if (ep->tx_expected_valid) CONSOLE("%u)", ep->tx_expected);
        else CONSOLE(".)");
        CONSOLE("\n");

        // if there is still something to send
        // and if expected, there is still till expected
        if ((ep->tx_sent < ep->tx_data_size) &&
            ((!ep->tx_expected_valid) ||
             (ep->tx_sent < ep->tx_expected))) 
        {
            // send more
            
            if (ep->utype == ep_utype_control)
            {
                // this is because ST ref man says
                // the reverse direction should stall in data stages
                // but only the last stage should set it to nak
                if ((ep->tx_sent + ep->mps) > ep->tx_data_size)
                {
                    hal5_usb_ep_set_status(
                            ep,
                            ep_status_nak,
                            ep_status_valid);
                }
                else
                {
                    hal5_usb_ep_set_status(
                            ep,
                            ep_status_stall,
                            ep_status_valid);
                }
            }
            else
            {
                hal5_usb_ep_set_status(
                        ep,
                        ep_status_nak,
                        ep_status_valid);
            }
        }
        else
        {
            // this is because the first get_descriptor.device
            // request does not wait or want a IN_0 
            if ((usb_device_state != usb_device_state_default) &&
                (ep->txbd->count == ep->mps))
            {
                hal5_usb_ep_prepare_for_in(
                        ep,
                        ep_status_nak,
                        NULL,
                        0,
                        false,
                        0);
            }
            else
            {
                // done
                hal5_usb_device_in_stage_completed(ep);
            }
        }
    }
    else
    {
        // SETUP, OUT or IN transaction is not completed, not ACKed
        // so either a NAK or STALL received
        CONSOLE("usb_transaction_error\n");
        assert (false);
    }
}

static void hal5_usb_device_bus_error(void)
{
    CONSOLE("usb_bus_error\n");
}

static void hal5_usb_device_bus_reset(void)
{
    CONSOLE("usb_bus_reset\n");

    // USB BUS RESET does not happen only once before setup
    // it also happens before setting the address during setup

    switch (usb_device_state)
    {
        case usb_device_state_default: 
            break;

        case usb_device_state_address:
        case usb_device_state_configured:
            // after the first enumeration
            // windows start a second enumeration with a bus reset
            // so device should continue functioning
            // but naturally with address=0 at default state
            // more info here: https://techcommunity.microsoft.com/t5/microsoft-usb-blog/how-does-usb-stack-enumerate-a-device/ba-p/270685
            usb_device_state = usb_device_state_default;
            USB_DRD_FS->DADDR = USB_DADDR_EF;
            break;
    }

    // endpoint 0 is a control endpoint
    // so it works in both directions
    // free in case it is allocated before
    hal5_usb_ep_free(endpoints[0][0]);
    hal5_usb_ep_free(endpoints[0][1]);
    // next_bd_addr=64 because the first 64 bytes are buffer descriptor table
    hal5_usb_endpoint_t* ep = hal5_usb_ep_create(
            NULL,
            64);

    endpoints[0][0] = ep;
    endpoints[0][1] = ep;

    hal5_usb_ep_sync_from_reg(ep);

    hal5_usb_ep_prepare_for_out(
            ep, 
            ep_status_stall);

    // this is the only place sync is done manually
    // all other transactions are automatically synced
    // when returned from trx_completed callback in USB INT handler
    hal5_usb_ep_sync_to_reg(ep);

}

static void hal5_usb_device_suspend(void)
{
    CONSOLE("usb_suspend\n");
}

static void hal5_usb_device_wakeup(void)
{
    CONSOLE("usb_wakeup\n");
}

static void hal5_usb_device_buffer_overflow(void)
{
    CONSOLE("usb_buffer_overflow\n");
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
        USB_DRD_FS->ISTR &= ~USB_ISTR_RESET_Msk;
        USB_DRD_FS->ISTR &= ~USB_ISTR_SUSP_Msk;
        hal5_usb_device_bus_reset();
    } 
    else if (istr & USB_ISTR_CTR) 
    {
        // transfer completed (ACKed, NAKed or STALLed)
        // this interrupt is called after USB transaction is finished
        
        // transaction means 
        // a token (SETUP, IN, OUT)
        // zero or more data (DATA0, DATA1)
        // handshake (ACK, NAK, STALL)
        
        // ISTR CTR bit is read-only
        // no need to clear any bit in ISTR

        const uint8_t idn = (istr & USB_ISTR_IDN_Msk) & 0xF;
        const bool dir_out = (istr & USB_ISTR_DIR_Msk);

        hal5_usb_endpoint_t* ep = endpoints[idn][dir_out ? 1 : 0];
        assert (ep != NULL);

        hal5_usb_ep_sync_from_reg(ep);
        CONSOLE("\n<<<<<<\n");

        switch (hal5_usb_device_get_state())
        {
            case usb_device_state_configured:
                CONSOLE("configured\n");
                break;

            case usb_device_state_address:
                CONSOLE("address\n");
                break;

            case usb_device_state_default:
                CONSOLE("default\n");
                break;
        }

        if (dir_out)
        {
            CONSOLE("(out, %u, %u)\n", 
                    ep->rxbd->count,
                    ep->rx_received);
            uint32_t rx_count = hal5_usb_device_copy_from_endpoint(ep);
            ep->rx_received += rx_count;
            hal5_usb_device_transaction_completed(ep);
        }
        else // dir_in
        {
            CONSOLE("(in, %u)\n", ep->txbd->count);
            ep->tx_sent += ep->txbd->count;
            hal5_usb_device_transaction_completed(ep);
        }

        if (ep->tx_status == ep_status_valid)
        {
            CONSOLE("TX (%u, %u, %u/%u-", 
                    ep->mps,
                    ep->txbd->count,
                    ep->tx_sent,
                    ep->tx_data_size);
            if (ep->tx_expected_valid) CONSOLE("%u)", ep->tx_expected);
            else CONSOLE(".)");

            uint32_t tx_count = hal5_usb_device_copy_to_endpoint(ep);

            CONSOLE(" %lu\n", tx_count);
        }

        //hal5_usb_ep_dump_status(ep);
        CONSOLE(">>>>>>\n");
        hal5_usb_ep_sync_to_reg(ep);
    } 
    else if (istr & USB_ISTR_PMAOVR) 
    {
        // PMA overrun/underrun detected
        
        // avoid read-modify-write of ISTR
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
        // wake-up signalling detected
        // SUSPRDY is automatically cleared
        
        // avoid read-modify-write of ISTR
        // clear WKUP
        USB_DRD_FS->ISTR = ~(1 << USB_ISTR_WKUP_Pos);

        // turn on external oscillators and device PLL etc.
        hal5_usb_device_wakeup();
        // clear SUSPEN so suspend check is enabled
        CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_SUSPEN);
    } 
    else if (istr & USB_ISTR_SUSP) 
    {
        // suspend detected
        // no activity (no SOF) for >3ms
        // SUSP flag is still set for reset as well
        // so check SUSP flag after checking RESET

        // set SUSPEN enabled
        //  so suspend condition is not checked and 
        //  SUSP interrupt is not repeatedly called 
        SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_SUSPEN);

        // avoid read-modify-write of ISTR
        // clear SUSP
        USB_DRD_FS->ISTR = ~(1 << USB_ISTR_SUSP_Pos);

        // remove power from USB transceivers
        SET_BIT(USB_DRD_FS->CNTR, USB_CNTR_SUSPRDY);

        // turn off external oscillators and device PLL etc.
        hal5_usb_device_suspend();
    } 
    else 
    {
        CONSOLE("UNKNOWN INTERRUPT: ISTR: 0x%08lX\n", istr);
        assert (false);
    }
}

void hal5_usb_device_connect(void) 
{
    hal5_usb_device_reset();

    CONSOLE("USB connect: pulling-up D+\n");

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

    CONSOLE("USB disconnect: pull-up removed from D+\n");
    
    // hold reset
    CLEAR_BIT(USB_DRD_FS->CNTR, USB_CNTR_USBRST);

    CONSOLE("USB disconnect: holding USBRST\n");
}


