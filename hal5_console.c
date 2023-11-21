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

static bool console_exists = false;

static uint32_t baud;

void hal5_console_configure(
        const uint32_t b,
        const bool disable_stdio_buffer)
{
    baud = b;
    hal5_lpuart_configure(baud);
    console_exists = true;
    if (disable_stdio_buffer) 
    {
        // disable stdout buffering
        setvbuf(stdout, NULL, _IONBF, 0);
    }
}

void hal5_console_dump_info() 
{
    CONSOLE("Console is LPUART1. %lu, 8N1.\n", baud);
}

void hal5_console_write(
        const char ch)
{
    hal5_lpuart_write(ch);
}

bool hal5_console_read(
        char* ch)
{
    return hal5_lpuart_read(ch);
}

void hal5_console_clearscreen(void) {
    CONSOLE("\e[2J");
}

static void hal5_console_setfgcolor(
        const int c) {
    CONSOLE("\e[3%dm", c);
}

void hal5_console_normal_colors(void) {
    // white
    hal5_console_setfgcolor(7);
}

void hal5_console_boot_colors(void) {
    // red
    hal5_console_setfgcolor(1);
}

void hal5_console_clear_line(void) 
{
    CONSOLE("\e[2K");
}

void hal5_console_move_cursor(
        const uint32_t x, 
        const uint32_t y) 
{
    CONSOLE("\e[%lu;%luH", y, x);
}

void hal5_console_move_cursor_up(
        const uint32_t nlines) 
{
    CONSOLE("\e[%luA", nlines);
}

void hal5_console_move_cursor_down(
        const uint32_t nlines) 
{
    CONSOLE("\e[%luB", nlines);
}

void hal5_console_move_cursor_left(
        const uint32_t nlines) 
{
    CONSOLE("\e[%luC", nlines);
}

void hal5_console_move_cursor_right(
        const uint32_t nlines) 
{
    CONSOLE("\e[%luD", nlines);
}

void hal5_console_save_cursor(void)
{
    CONSOLE("\e7");
}

void hal5_console_restore_cursor(void)
{
    CONSOLE("\e8");
}

void hal5_console_heartbeat(void)
{
    hal5_console_save_cursor();
    switch (hal5_slow_ticks % 4) 
    {
        case 0: CONSOLE("-"); break;
        case 1: CONSOLE("\\"); break;
        case 2: CONSOLE("|"); break;
        case 3: CONSOLE("/"); break;
    }
    hal5_console_restore_cursor();
}
