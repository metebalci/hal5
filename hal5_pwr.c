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

#include <stdio.h>

#include <stm32h5xx.h>

#include "hal5.h"
#include "hal5_private.h"

void hal5_pwr_enable_usb33(void) 
{
    // enable USB voltage level detector
    // this should be enabled before checking USB33RDY
    SET_BIT(PWR->USBSCR, PWR_USBSCR_USB33DEN);
    // wait until USB 3.3V supply is ready
    while ((PWR->VMSR & PWR_VMSR_USB33RDY_Msk) == 0);
    // USB supply is valid
    SET_BIT(PWR->USBSCR, PWR_USBSCR_USB33SV);
}

hal5_pwr_voltage_scaling_t hal5_pwr_get_voltage_scaling()
{
    const uint32_t current = (PWR->VOSSR & PWR_VOSSR_ACTVOS_Msk) 
        >> PWR_VOSSR_ACTVOS_Pos;

    if (current == 0b00) return hal5_pwr_vos3;
    else if (current == 0b01) return hal5_pwr_vos2;
    else if (current == 0b10) return hal5_pwr_vos1;
    else if (current == 0b11) return hal5_pwr_vos0;
    else assert (false);
}

void hal5_pwr_change_voltage_scaling(hal5_pwr_voltage_scaling_t vos)
{
    const hal5_pwr_voltage_scaling_t current = 
        hal5_pwr_get_voltage_scaling();

    if (vos == current) return;

    uint32_t vos_bits;

    switch (vos)
    {
        case hal5_pwr_vos3: vos_bits = 0b00; break;
        case hal5_pwr_vos2: vos_bits = 0b01; break;
        case hal5_pwr_vos1: vos_bits = 0b10; break;
        case hal5_pwr_vos0: vos_bits = 0b11; break;
        default: assert (false);
    }

    MODIFY_REG(PWR->VOSCR, PWR_VOSCR_VOS_Msk, 
            vos_bits << PWR_VOSCR_VOS_Pos);

    // if voltage increased, wait for it
    // if vos0, definitely increased
    // if vos1, increased if current is vos2 or vos3
    // if vos2, increase if current is vos3
    if ((vos == hal5_pwr_vos0) ||
            ((vos == hal5_pwr_vos1) && 
             ((current == hal5_pwr_vos2) || 
              (current == hal5_pwr_vos3))) ||
            ((vos == hal5_pwr_vos2) && 
             (current == hal5_pwr_vos3))) 
    {
        while ((PWR->VOSSR & PWR_VOSSR_VOSRDY_Msk) == 0);
    }
}
