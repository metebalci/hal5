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

void hal5_rng_enable() 
{
    hal5_rcc_enable_hsi48();
    hal5_rcc_enable_rng();

    // make rng_clk hsi48_ker_ck 
    // already default after reset
    MODIFY_REG(RCC->CCIPR5, RCC_CCIPR5_RNGSEL_Msk, 0);

    SET_BIT(RNG->CR, RNG_CR_RNGEN);
}

uint32_t hal5_rng_random()
{
    uint32_t dr = 0;

    // first wait until status is OK and data is ready
    // then read the data and check if it is non-zero
    while (dr == 0) {

        uint32_t sr = 0;

        while ((sr & 0x7) != 0x1) {
            sr = RNG->SR;
        }

        dr = RNG->DR;

    }

    return dr;
}
