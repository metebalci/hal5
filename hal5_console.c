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

void hal5_console_configure(
        const uint32_t baud)
{
    hal5_lpuart_configure(baud);
    console_exists = true;
    // disable stdout buffering
    setvbuf(stdout, NULL, _IONBF, 0);
}

void hal5_console_dump_info() 
{
    printf("Console is LPUART1.\n");
    hal5_lpuart_dump_info();
}

void hal5_console_write(
        const char ch)
{
    if (!console_exists) return;
    hal5_lpuart_write(ch);
}

bool hal5_console_read(
        char* ch)
{
    if (!console_exists) return false;
    return hal5_lpuart_read(ch);
}

void hal5_console_clearscreen(void) {
    printf("\e[2J");
}

static void hal5_console_setfgcolor(
        const int c) {
    printf("\e[3%dm", c);
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
    printf("\e[2K");
}

void hal5_console_move_cursor(
        const uint32_t x, 
        const uint32_t y) 
{
    printf("\e[%lu;%luH", y, x);
}

void hal5_console_move_cursor_up(
        const uint32_t nlines) 
{
    printf("\e[%luA", nlines);
}

void hal5_console_move_cursor_down(
        const uint32_t nlines) 
{
    printf("\e[%luB", nlines);
}

void hal5_console_move_cursor_left(
        const uint32_t nlines) 
{
    printf("\e[%luC", nlines);
}

void hal5_console_move_cursor_right(
        const uint32_t nlines) 
{
    printf("\e[%luD", nlines);
}

void hal5_console_save_cursor(void)
{
    printf("\e7");
}

void hal5_console_restore_cursor(void)
{
    printf("\e8");
}

void hal5_console_heartbeat(void)
{
    hal5_console_save_cursor();
    switch (hal5_slow_ticks % 4) 
    {
        case 0: printf("-"); break;
        case 1: printf("\\"); break;
        case 2: printf("|"); break;
        case 3: printf("/"); break;
    }
    hal5_console_restore_cursor();
}
