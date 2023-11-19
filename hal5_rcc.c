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

hal5_rcc_reset_status_t hal5_rcc_get_reset_status(void)
{
    const uint32_t rsr = RCC->RSR;

    if (rsr & RCC_RSR_LPWRRSTF) {
        return reset_status_illegal_stop_entry;
    } else if (rsr & RCC_RSR_WWDGRSTF) {
        return reset_status_window_watchdog;
    } else if (rsr & RCC_RSR_IWDGRSTF) {
        return reset_status_independent_watchdog;
    } else if (rsr & RCC_RSR_SFTRSTF) {
        return reset_status_system_reset_by_cpu;
    } else if (rsr & RCC_RSR_BORRSTF) {
        return reset_status_bor;
    } else if (rsr & RCC_RSR_PINRSTF) {
        return reset_status_pin;
    } else {
        return reset_status_unknown;
    }

    SET_BIT(RCC->RSR, RCC_RSR_RMVF);
}

void hal5_rcc_change_hsidiv(hal5_rcc_hsidiv_t hsidiv)
{
    uint32_t hsidiv_bits;

    switch (hsidiv)
    {
        case hsidiv_1: hsidiv_bits = 0b00; break;
        case hsidiv_2: hsidiv_bits = 0b01; break;
        case hsidiv_4: hsidiv_bits = 0b10; break;
        case hsidiv_8: hsidiv_bits = 0b11; break;
        default: assert (false);
    }

    MODIFY_REG(RCC->CR, RCC_CR_HSIDIV_Msk,
            hsidiv_bits << RCC_CR_HSIDIV_Pos);

    while ((RCC->CR & RCC_CR_HSIDIVF) == 0);
}

bool hal5_rcc_is_csi_enabled()
{
    return ((RCC->CR & RCC_CR_CSIRDY) != 0);
}

bool hal5_rcc_is_lse_enabled()
{
    return ((RCC->BDCR & RCC_BDCR_LSERDY) != 0);
}

bool hal5_rcc_is_lsi_enabled()
{
    return ((RCC->BDCR & RCC_BDCR_LSIRDY) != 0);
}

bool hal5_rcc_is_hse_enabled()
{
    return ((RCC->CR & RCC_CR_HSERDY) != 0);
}

bool hal5_rcc_is_hsi_enabled()
{
    return ((RCC->CR & RCC_CR_HSIRDY) != 0);
}

bool hal5_rcc_is_hsi48_enabled()
{
    return ((RCC->CR & RCC_CR_HSI48RDY) != 0);
}

void hal5_rcc_enable_csi(void)
{
    if (!hal5_rcc_is_csi_enabled()) {
        SET_BIT(RCC->CR, RCC_CR_CSION);
        // wait until ready
        while ((RCC->CR & RCC_CR_CSIRDY) == 0);
    }
}

void hal5_rcc_enable_lse_bypass(void) { assert(false); }
void hal5_rcc_enable_lse_crystal(void) { assert(false); }

void hal5_rcc_enable_lsi(void) { assert(false); }

void hal5_rcc_enable_hsi(void) 
{
    if (!hal5_rcc_is_hsi_enabled())
    {
        SET_BIT(RCC->CR, RCC_CR_HSION);
        while ((RCC->CR & RCC_CR_HSIRDY) == 0);
    }
}

void hal5_rcc_enable_hse_bypass(void) { assert(false); }
void hal5_rcc_enable_hse_crystal(void) { assert(false); }

void hal5_rcc_enable_hsi48(void) 
{
    if (!hal5_rcc_is_hsi48_enabled())
    {
        SET_BIT(RCC->CR, RCC_CR_HSI48ON);
        while ((RCC->CR & RCC_CR_HSI48RDY) == 0);
    }
}

void hal5_rcc_enable_gpio_port_by_index(uint32_t port_index)
{
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN << port_index);
}

void hal5_rcc_enable_lpuart1() {
    SET_BIT(RCC->APB3ENR, RCC_APB3ENR_LPUART1EN);
}

void hal5_rcc_enable_rng() {
    SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_RNGEN);
}

void hal5_rcc_enable_usb() 
{
    SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USBEN);
}

void hal5_rcc_enable_mco2(
        hal5_rcc_mco2sel_t src,
        uint32_t prescaler)
{
    assert (prescaler <= 0xF);

    uint32_t mco2sel_bits;

    switch (src)
    {
        case mco2sel_sysclk:  mco2sel_bits=0b0000; break;
        case mco2sel_pll2:    mco2sel_bits=0b0001; break;
        case mco2sel_hse:     mco2sel_bits=0b0010; break;
        case mco2sel_pll1:    mco2sel_bits=0b0011; break;
        case mco2sel_csi:     mco2sel_bits=0b0100; break;
        case mco2sel_lsi:     mco2sel_bits=0b0101; break;
        default: assert (false);
    }

    MODIFY_REG(RCC->CFGR1, RCC_CFGR1_MCO2SEL_Msk,
            mco2sel_bits << RCC_CFGR1_MCO2SEL_Pos);

    MODIFY_REG(RCC->CFGR1, RCC_CFGR1_MCO2PRE_Msk,
            prescaler << RCC_CFGR1_MCO2PRE_Pos);

    // MCO2 is PC9 AF0
    hal5_gpio_configure_as_af(
            PC9,
            af_pp_floating,
            very_high_speed,
            AF0);
}

void hal5_rcc_change_sys_ck_src(hal5_rcc_sys_ck_src_t src)
{ 
    uint32_t src_bits;

    switch (src)
    {
        case sys_ck_src_hsi: src_bits=0b00; break;
        case sys_ck_src_csi: src_bits=0b01; break;
        case sys_ck_src_hse: src_bits=0b10; break;
        case sys_ck_src_pll1: src_bits=0b11; break;
        default: assert (false);
    }

    MODIFY_REG(RCC->CFGR1, RCC_CFGR1_SW_Msk,
            src_bits << RCC_CFGR1_SW_Pos);

    while (((RCC->CFGR1 & RCC_CFGR1_SWS_Msk) >> RCC_CFGR1_SWS_Pos) != src_bits);
}

bool hal5_rcc_search_pll_config_integer_mode(
        const uint32_t src_ck,
        const uint32_t target_p_ck,
        const uint32_t target_q_ck,
        const uint32_t target_r_ck,
        bool only_even_p,
        uint32_t* divm,
        uint32_t* muln,
        uint32_t* divp,
        uint32_t* divq,
        uint32_t* divr)
{
    for (uint32_t m = 1; m <= 63; m++)
    {
        const uint32_t ref_ck = src_ck / m;

        // ref_ck has to be between 1-16MHz
        if (ref_ck <  1000000) continue;
        if (ref_ck > 16000000) continue;

        // 1,2,3 are reserved
        for (uint32_t n = 4; n <= 512; n++)
        {
            const uint32_t vco_ck = ref_ck * n;

            if (vco_ck < 150000000) continue;
            if (vco_ck > 836000000) continue;

            uint32_t p = 2;
            bool pfound = false;

            uint32_t q = 1;
            bool qfound = false;

            uint32_t r = 1;
            bool rfound = false;

            if (target_p_ck > 0)
            {
                while (p <= 128)
                {
                    const uint32_t pll_p_ck = vco_ck / p;
                    if (pll_p_ck == target_p_ck)
                    {
                        pfound = true;
                        break;
                    }
                    if (only_even_p) p += 2;
                    else p++;
                }
            } 
            else 
            {
                p = 0;
                pfound = true;
            }

            if (target_q_ck > 0)
            {
                for (uint32_t q = 1; q <= 128; q++)
                {
                    const uint32_t pll_q_ck = vco_ck / q;
                    if (pll_q_ck == target_q_ck)
                    {
                        qfound = true;
                        break;
                    }
                }
            } 
            else 
            {
                q = 0;
                qfound = true;
            }

            if (target_r_ck > 0)
            {
                for (uint32_t r = 1; r <= 128; r++)
                {
                    const uint32_t pll_r_ck = vco_ck / r;
                    if (pll_r_ck == target_r_ck)
                    {
                        rfound = true;
                        break;
                    }

                }
            } 
            else 
            {
                r = 0;
                rfound = true;
            }

            if (pfound && qfound && rfound)
            {
                *divm = m;
                *muln = n;
                if (divp != NULL) *divp = p;
                if (divq != NULL) *divq = q;
                if (divr != NULL) *divr = r;
                return true;
            }
        }
    }

    return false;
}

void hal5_rcc_initialize_pll1_integer_mode(
        const hal5_rcc_pll_src_t src,
        const uint32_t divm, 
        const uint32_t muln,
        const uint32_t divp, 
        const uint32_t divq, 
        const uint32_t divr,
        const bool pen, 
        const bool qen, 
        const bool ren)
{
    // 0 <= divm <= 63
    // 0 means prescaler disabled
    assert (divm >= 0);
    assert (divm <= 63);

    // 4 <= divn <= 512
    assert (muln >= 4);
    assert (muln <= 512);

    // divp odd factors not allowed, and <= 128
    assert (divp > 0);
    assert (divp % 2 == 0);
    assert (divp <= 128);

    // 1 <= divq <= 128
    assert (divq > 0);
    assert (divq <= 128);

    // 1 <= divr <= 128
    assert (divr > 0);
    assert (divr <= 128);

    // m is m
    uint32_t m = divm;
    // muln=4 is encoded as 3
    uint32_t n = muln-1;
    // divp=2 is encoded as 1
    uint32_t p = divp-1; 
    // divq=1 is encoded as 0
    uint32_t q = divq-1;
    // divr=1 is encoded as 0
    uint32_t r = divr-1;

    // ref: RM0481
    // Figure 52. PLLs initialization flow

    // set PLL src
    uint32_t src_bits;
    switch (src)
    {
        case pll_src_hsi: src_bits = 0b01; break;
        case pll_src_csi: src_bits = 0b10; break;
        case pll_src_hse: src_bits = 0b11; break;
        default: assert (false);
    }

    MODIFY_REG(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1SRC_Msk,
            src_bits << RCC_PLL1CFGR_PLL1SRC_Pos);

    MODIFY_REG(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1M_Msk,
            m << RCC_PLL1CFGR_PLL1M_Pos);

    uint32_t ref_ck;
    if (src == pll_src_hsi) 
    {
        assert (hal5_rcc_is_hsi_enabled());
        ref_ck = hal5_rcc_get_hsi_ck();
    }
    else if (src == pll_src_csi) 
    {
        assert (hal5_rcc_is_csi_enabled());
        ref_ck = hal5_rcc_get_csi_ck();
    }
    else if (src == pll_src_hse) 
    {
        assert (hal5_rcc_is_hse_enabled());
        ref_ck = hal5_rcc_get_hse_ck();
    }
    else assert (false);
    // m == 0 means prescaler is disabled, so effectively equals to m=1
    if (m > 0) ref_ck = ref_ck / m;

    // it says if ref_ck < 2MHz, smaller range should be selected
    if (ref_ck < 2000000) 
    {
        SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1VCOSEL);
    }
    else
    {
        CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1VCOSEL);
    }

    uint32_t rge;
    if (ref_ck <= 2000000) 
    {
        rge = 0b00;
    }
    else if (ref_ck <= 4000000) 
    {
        rge = 0b01;
    }
    else if (ref_ck <= 8000000) 
    {
        rge = 0b10;
    }
    else if (ref_ck <= 16000000) 
    {
        rge = 0b11;
    }
    else 
    {
        assert (false);
    }

    // set PLL input freq. range
    MODIFY_REG(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1RGE_Msk,
            rge << RCC_PLL1CFGR_PLL1RGE_Pos);

    // disable fractional part
    CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1FRACEN);

    // enable/disable P output
    if (pen) SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1PEN);
    else CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1PEN);

    // enable/disable Q output
    if (qen) SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1QEN);
    else CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1QEN);

    // enable/disable R output
    if (ren) SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1REN);
    else CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1REN);

    // set div N, P, Q, R
    RCC->PLL1DIVR = (RCC->PLL1DIVR & 0x80800000) |
        ((r << RCC_PLL1DIVR_PLL1R_Pos) |
         (q << RCC_PLL1DIVR_PLL1Q_Pos) |
         (p << RCC_PLL1DIVR_PLL1P_Pos) |
         (n << RCC_PLL1DIVR_PLL1N_Pos));

    // enable PLL
    SET_BIT(RCC->CR, RCC_CR_PLL1ON);

    // wait for PLL to lock
    while ((RCC->CR & RCC_CR_PLL1RDY_Msk) == 0);
}

void hal5_rcc_change_lpuart1_ker_ck(hal5_rcc_lpuart1sel_t src)
{
    uint32_t src_bits;

    switch (src)
    {
        case lpuart1sel_pclk3:      src_bits=0b000; break;
        case lpuart1sel_pll2_q_ck:  src_bits=0b001; break;
        case lpuart1sel_pll3_q_ck:  src_bits=0b010; break;
        case lpuart1sel_hsi_ker_ck: src_bits=0b011; break;
        case lpuart1sel_csi_ker_ck: src_bits=0b100; break;
        case lpuart1sel_lse_ck:     src_bits=0b101; break;
        default: assert (false);
    }

    MODIFY_REG(RCC->CCIPR3, RCC_CCIPR3_LPUART1SEL_Msk,
            src_bits << RCC_CCIPR3_LPUART1SEL_Pos);
}
