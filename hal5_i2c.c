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

#include <stdbool.h>
#include <stdio.h>

#include <stm32h5xx.h>

#include "hal5.h"

static I2C_TypeDef* const i2c = I2C2;

void hal5_i2c_configure()
{
    // I2C2 uses pclk1 by default

    // configure pins
    // PF0 I2C2_SCL, PF0 I2C2_SDA with AF4
    hal5_gpio_configure_as_af(
            PF0,
            af_od_floating,
            high_speed,
            AF4);

    hal5_gpio_configure_as_af(
            PF1,
            af_od_floating,
            high_speed,
            AF4);

    // enable I2C1 peripheral clock
    SET_BIT(RCC->APB1LENR, RCC_APB1LENR_I2C2EN);

    const uint32_t tscll    = 5000; // ns
    const uint32_t tsclh    = 5000; // ns
    const uint32_t tsdadel  = 1000; // ns
    const uint32_t tscldel  = 1000; // ns

    const uint32_t presc = 15;
    assert (presc <= 0xF);

    const double tpresc = (1000000000.0 / (hal5_rcc_get_i2c_ker_ck(2) / (presc + 1))); // ns

    const uint32_t scldel = tscldel / tpresc;
    assert (scldel <= 0xF);

    const uint32_t sdadel = tsdadel / tpresc;
    assert (sdadel <= 0xF);

    const uint32_t scll = (tscll / tpresc) - 1;
    assert (scll <= 0xF);

    const uint32_t sclh = (tsclh / tpresc) - 1;
    assert (sclh <= 0xF);

    MODIFY_REG(i2c->TIMINGR, I2C_TIMINGR_PRESC_Msk,
            presc << I2C_TIMINGR_PRESC_Pos);

    MODIFY_REG(i2c->TIMINGR, I2C_TIMINGR_SCLDEL_Msk,
            scldel << I2C_TIMINGR_SCLDEL_Pos);

    MODIFY_REG(i2c->TIMINGR, I2C_TIMINGR_SDADEL_Msk,
            sdadel << I2C_TIMINGR_SDADEL_Pos);

    MODIFY_REG(i2c->TIMINGR, I2C_TIMINGR_SCLH_Msk,
            scll << I2C_TIMINGR_SCLH_Pos);

    MODIFY_REG(i2c->TIMINGR, I2C_TIMINGR_SCLL_Msk,
            scll << I2C_TIMINGR_SCLL_Pos);

    // enable I2C
    SET_BIT(i2c->CR1, I2C_CR1_PE);
}

bool hal5_i2c_read(
        uint8_t* ch)
{
    // anything in RXDR ?
    if (i2c->ISR & I2C_ISR_RXNE_Msk) {
        *ch = i2c->RXDR;
        return true;
    } else {
        return false;
    }
}

void hal5_i2c_write(
        const uint8_t ch)
{
    // wait until TXDR is empty
    while ((i2c->ISR & I2C_ISR_TXE_Msk) == 0);
    i2c->TXDR = ch;
}
