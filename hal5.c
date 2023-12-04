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

void hal5_set_vector(
        uint32_t vector_number,
        void (*vector)(void))
{
    typedef void (*interrupt_handler)(void);
    interrupt_handler* vectors = (interrupt_handler*) SCB->VTOR;
    vectors[15] = vector;
    __DSB();
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
        const uint32_t target_ck,
        uint32_t* divm_out,
        uint32_t* muln_out,
        uint32_t* divp_out)
{
    uint32_t divm, muln, divp;
    bool pll_config_found = hal5_rcc_search_pll_config_integer_mode(
            hal5_rcc_get_hsi_ck(),
            target_ck, 0, 0, true,
            &divm, &muln, &divp, NULL, NULL);

    if (pll_config_found)
    {
        if (divm_out != NULL) *divm_out = divm;
        if (muln_out != NULL) *muln_out = muln;
        if (divp_out != NULL) *divp_out = divp;

        hal5_rcc_initialize_pll1_integer_mode(
                pll_src_hsi,
                divm, muln, divp, divp, divp,
                true, false, false);

        hal5_change_sys_ck(sys_ck_src_pll1);
    }
    else
    {
        CONSOLE("PLL config not found.\n");
        assert (false);
    }

}

static GPIO_TypeDef* debug_pin_port = 0;
static uint32_t debug_pin_set       = 0;
static uint32_t debug_pin_reset     = 0;

void hal5_debug_configure(const hal5_gpio_pin_t pin)
{
    hal5_gpio_configure_as_output(
            pin,
            output_pp_floating,
            high_speed);

    hal5_gpio_reset(pin);

    const uint32_t port_index = (pin >> 8) & 0xFF;

    debug_pin_port = (GPIO_TypeDef*) (GPIOA_BASE + 0x0400UL * port_index);

    const uint32_t pin_number = pin & 0xFF;

    debug_pin_set   = pin_number;
    debug_pin_reset = pin_number << 16;
}

inline void hal5_debug_pulse()
{
    // PA0 set then reset
    debug_pin_port->BSRR = debug_pin_set;
    __DSB(); // make sure the above completed
    debug_pin_port->BSRR = debug_pin_reset;
    __DSB(); // make sure the above completed
}
