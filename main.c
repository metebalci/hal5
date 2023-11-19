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
#include "hal5_usb_device.h"

void HardFault_Handler(void)
{
    bsp_fault();
    hal5_dump_fault_info();
    hal5_freeze();
}

void button_callback(void)
{
}

void boot(void)
{
    // configure console as early as possible
    // console uses LPUART1 running with LSI
    hal5_console_configure(921600);

    // clear screen and set fg color to red
    // boot messages are shown in red
    hal5_console_clearscreen();
    hal5_console_boot_colors();

    hal5_console_dump_info();

    CONSOLE("Booting...\n");

    hal5_icache_enable();
    CONSOLE("ICACHE enabled.\n");

    hal5_flash_enable_prefetch();
    CONSOLE("Prefetch enabled.\n");

    // configure bsp first
    // required for showing progress (on leds)
    // might be required if external clocks are used
    bsp_configure(button_callback);

    hal5_watchdog_configure(5000);

    hal5_rcc_initialize_pll1(
            pll_src_hsi,
            4, 60, 2, 4, 2,
            true, false, false);
    CONSOLE("PLL1 initialized.\n");

    hal5_change_sys_ck(sys_ck_src_pll1);
    CONSOLE("SYSCLK is now PLL1_P.\n");

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
    CONSOLE("USB configured.\n");
    hal5_usb_device_configure();
    CONSOLE("USB device configured.\n");

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
