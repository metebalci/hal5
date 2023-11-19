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

void hal5_dump_fault_info(void)
{
    CONSOLE("Fault Status:");
    if (SCB->CFSR & SCB_CFSR_DIVBYZERO_Msk) CONSOLE(" DIVBYZERO");
    if (SCB->CFSR & SCB_CFSR_UNALIGNED_Msk) CONSOLE(" UNALIGNED");
    if (SCB->CFSR & SCB_CFSR_STKOF_Msk) CONSOLE(" STKOF");
    if (SCB->CFSR & SCB_CFSR_NOCP_Msk) CONSOLE(" NOCP");
    if (SCB->CFSR & SCB_CFSR_INVPC_Msk) CONSOLE(" INVPC");
    if (SCB->CFSR & SCB_CFSR_INVSTATE_Msk) CONSOLE(" INVSTATE");
    if (SCB->CFSR & SCB_CFSR_UNDEFINSTR_Msk) CONSOLE(" UNDEFINSTR");
    CONSOLE("\n");
}

void hal5_freeze() 
{
    CONSOLE("Program will freeze now keeping iWatchDog alive.\n");
    CONSOLE("You have to manually reset.\n");

    while (1) hal5_watchdog_heartbeat();
}

void hal5_change_sys_ck(
        const hal5_rcc_sys_ck_src_t src)
{
    uint32_t target_freq;

    switch (src)
    {
        case sys_ck_src_hsi: target_freq = hal5_rcc_get_hsi_ck(); break;
        case sys_ck_src_csi: target_freq = hal5_rcc_get_csi_ck(); break;
        case sys_ck_src_hse: target_freq = hal5_rcc_get_hse_ck(); break;
        case sys_ck_src_pll1: target_freq = hal5_rcc_get_pll1_p_ck(); break;
        default: assert (false);
    }

    hal5_flash_latency_t latency;
    hal5_pwr_voltage_scaling_t vos;
    const bool flash_ok = hal5_flash_calculate_latency(
            target_freq / 1000000,
            true,
            &latency, &vos);

    assert (flash_ok);

    if (target_freq > hal5_rcc_get_sys_ck())
    {
        // freq increasing

        // first change the flash latency
        hal5_flash_change_latency(latency);

        // then change voltage scaling
        hal5_pwr_change_voltage_scaling(vos);

        // finally change the freq
        hal5_rcc_change_sys_ck_src(src);
    }
    else
    {
        // freq decreasing

        // first change freq
        hal5_rcc_change_sys_ck_src(src);

        // then voltage scaling
        hal5_pwr_change_voltage_scaling(vos);

        // finally change the the flash latency
        hal5_flash_change_latency(latency);
    }
}

void hal5_change_sys_ck_to_pll1_p(
        const uint32_t target_ck)
{
    uint32_t divm, muln, divp;
    CONSOLE("Searching for PLL config with %lu for %lu...\n", 
            hal5_rcc_get_hsi_ck(), 
            target_ck);
    bool pll_config_found = hal5_rcc_search_pll_config_integer_mode(
            hal5_rcc_get_hsi_ck(),
            target_ck, 0, 0, true,
            &divm, &muln, &divp, NULL, NULL);

    if (pll_config_found)
    {
        CONSOLE("PLL config is found: /M=%lu xN=%lu /P=%lu.\n",
                divm, muln, divp);

        hal5_rcc_initialize_pll1_integer_mode(
                pll_src_hsi,
                divm, muln, divp, divp, divp,
                true, false, false);
        CONSOLE("PLL1 initialized.\n");

        hal5_change_sys_ck(sys_ck_src_pll1);
        CONSOLE("SYSCLK is now PLL1_P.\n");
    }
    else
    {
        CONSOLE("PLL config not found.\n");
    }

}

void hal5_debug_configure()
{
    hal5_gpio_configure_as_output(
            PA0,
            output_pp_floating,
            high_speed);
    hal5_gpio_reset(PA0);
}

inline void hal5_debug_pulse()
{
    // PA0 set then reset
    GPIOA->BSRR = 0x00000001UL;
    __DSB(); // make sure the above completed
    GPIOA->BSRR = 0x00010000UL;
    __DSB(); // make sure the above completed
}
