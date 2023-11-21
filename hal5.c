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

static const char* CFSR_BIT_DESCRIPTIONS[] = {
    // MMFSR, MemManage Fault Status Register
    "IACCVIOL (Instruction Access Violation)",
    "DACCVIOL (Data Access Violation)",
    "RESERVED",
    "MUNSTKERR (MemManage fault on unstacking for a return from exception)",
    "MSTKERR (MemManage fault on stacking for exception entry)",
    "MLSPERR (MemManage fault occurred during floating-point lazy state preservation)",
    "RESERVED",
    "MMARVALID (MMFAR holds a valid fault address)",

    // BFSR, BusFault Status Register
    "IBUSERR (Instruction bus error)",
    "PRECISERR (Precise data bus error)",
    "RESERVED",
    "UNSTKERR (BusFault on unstacking for a return from exception)",
    "STKERR (BusFault on stacking for exception entry)",
    "LSPERR (BusFault occurred during floating-point lazy state preservation)",
    "RESERVED",
    "BFARVALID (BFAR holds a valid fault address)",

    // UFSR, Usage Fault Register
    "UNDEFINSTR (Undefined instruction)",
    "INVSTATE (Invalid state)",
    "INVPC (Invalid PC)",
    "NOCP (No coprocessor)",
    "STKOF (Stack overflow)",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "UNALIGNED (Unaligned access)",
    "DIVBYZERO (Divide by zero)",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
    "RESERVED",
};

void hal5_dump_cfsr_info(void)
{
    const uint32_t cfsr = SCB->CFSR;

    CONSOLE("CFSR, Configurable Fault Status Register [0x%08lX]:\n", cfsr);

    for (uint32_t pos = 0; pos < 32; pos++)
    {
        if (cfsr & (1 << pos)) 
        {
            const char* desc = CFSR_BIT_DESCRIPTIONS[pos];
            CONSOLE("  %s\n", desc);

            // MMARVALID
            if (pos == SCB_CFSR_MMARVALID_Pos)
            {
                // MMFAR
                CONSOLE("    MMFAR=0x%08lX\n", SCB->MMFAR);
            }
            // BFARVALID
            else if (pos == SCB_CFSR_BFARVALID_Pos)
            {
                // BFAR
                CONSOLE("    BFAR=0x%08lX\n", SCB->BFAR);
            }
        }
    }
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
