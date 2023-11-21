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
#include <stdlib.h>

#include "bsp.h"
#include "hal5.h"

typedef __PACKED_STRUCT
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
} exception_stack_frame_t;

void HardFault_Callback(const void* stack_frame)
{
    const exception_stack_frame_t* sf = 
        (exception_stack_frame_t*) stack_frame;

    CONSOLE("HardFault pc=0x%08lX lr=0x%08lX\n", sf->pc, sf->lr);
    bsp_fault();
    hal5_dump_cfsr_info();
    hal5_freeze();
}

void button_callback(void)
{
}

void boot(void) {

    hal5_rcc_initialize();

    // configure console as early as possible
    // console uses LPUART1 running with LSI
    hal5_console_configure(921600, false);

    // clear screen and set fg color to red
    // boot messages are shown in red
    hal5_console_clearscreen();
    hal5_console_boot_colors();

    hal5_console_dump_info();

    CONSOLE("Booting...\n");

    const hal5_rcc_reset_status_t reset_status = 
        hal5_rcc_get_reset_status();

    switch (reset_status)
    {
        case reset_status_independent_watchdog:
            CONSOLE("Due to watchdog reset...\n");
            break;

        case reset_status_system_reset_by_cpu:
            CONSOLE("Due to CPU reset...\n");
            break;

        case reset_status_bor:
            CONSOLE("Due to brown-out reset...\n");
            break;

        default:
    }

    hal5_icache_enable();
    CONSOLE("ICACHE enabled.\n");

    hal5_flash_enable_prefetch();
    CONSOLE("Prefetch enabled.\n");

    // configure bsp first
    // required for showing progress (on leds)
    // might be required if external clocks are used
    bsp_configure(button_callback);

    hal5_watchdog_configure(5000);

    hal5_change_sys_ck_to_pll1_p(240000000);

    hal5_rcc_dump_clock_info();

    hal5_rcc_enable_mco2(mco2sel_sysclk, 0);
    CONSOLE("MCO2 shows SYSCLK.\n");

    hal5_systick_configure();
    CONSOLE("SYSTICK configured.\n");

    hal5_rng_enable();
    // initialize random from an RNG seed
    const uint32_t seed = hal5_rng_random();
    srand(seed);
    CONSOLE("RNG enabled. [%08lX]\n", seed);

    hal5_usb_configure();
    hal5_usb_device_configure();

    bsp_boot_completed();
    CONSOLE("Boot completed.\n");

    hal5_console_normal_colors();
}

int main(void) 
{
    boot();

    const bool flower = true;

    uint32_t last = hal5_slow_ticks;

    while (1) {

        hal5_watchdog_heartbeat();

        const uint32_t now = hal5_slow_ticks;
        if (now > last) {
            last = now;
            bsp_heartbeat();
            //mcu_console_heartbeat();
        }

        char ch;
        if (hal5_console_read(&ch)) {
            if (ch == 'c') hal5_usb_device_connect();
            else if (ch == 'd') hal5_usb_device_disconnect();
        }

    }

    // you shall not return
    assert (false);

    return 0;

}
