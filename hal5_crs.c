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

void hal5_crs_enable_for_usb(void)
{
    hal5_rcc_enable_hsi48();

    // reset
    SET_BIT(RCC->APB1LRSTR, RCC_APB1LRSTR_CRSRST);
    CLEAR_BIT(RCC->APB1LRSTR, RCC_APB1LRSTR_CRSRST);

    // enable clock recovery system to tune HSI48
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_CRSEN);
    // sync on rising edge 
    CLEAR_BIT(CRS->CFGR, CRS_CFGR_SYNCPOL);
    // sync with USB SOF
    // crs_sync_in_3 is USB SOF, 0b10
    MODIFY_REG(CRS->CFGR, CRS_CFGR_SYNCSRC_Msk,
            0b10 << CRS_CFGR_SYNCSRC_Pos);
    // reload = (ftarget/fsync) - 1
    // reload = (48000000/1000 - 1) = 47999
    MODIFY_REG(CRS->CFGR, CRS_CFGR_RELOAD_Msk,
            47999 << CRS_CFGR_RELOAD_Pos);
    // use default limit and trim 
    // enable auto trim
    SET_BIT(CRS->CR, CRS_CR_AUTOTRIMEN);
    // enable error counter
    SET_BIT(CRS->CR, CRS_CR_CEN);
}
