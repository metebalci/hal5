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

#ifndef __MCU_H__
#define __MCU_H__

#include <stdbool.h>

#include <stm32h5xx.h>

#include "hal5_types.h"

#include "hal5_usb.h"
#include "hal5_usb_device.h"

#ifdef __cplusplus
extern "C" {
#endif

void hal5_dump_fault_info(void);
void hal5_debug_configure(void);
void hal5_debug_pulse(void);
void hal5_freeze(void);
void hal5_change_sys_ck(
        const hal5_rcc_sys_ck_src_t src);

// CACHE

void hal5_icache_enable(void);

// CONSOLE

void hal5_console_configure(
        const uint32_t baud);

void hal5_console_dump_info(void);

void hal5_console_write(
        const char ch);

bool hal5_console_read(
        char* ch);

void hal5_console_clearscreen(void);
void hal5_console_boot_colors(void);
void hal5_console_normal_colors(void);
void hal5_console_save_cursor(void);
void hal5_console_restore_cursor(void);
void hal5_console_clear_line(void);

void hal5_console_move_cursor(
        const uint32_t x, 
        const uint32_t y);
void hal5_console_move_cursor_up(
        const uint32_t nlines);

void hal5_console_move_cursor_down(
        const uint32_t nlines);

void hal5_console_move_cursor_left(
        const uint32_t nlines);

void hal5_console_move_cursor_right(
        const uint32_t nlines);

// CRS

void hal5_crs_enable_for_usb(void);

// FLASH

bool hal5_flash_calculate_latency(
        const uint32_t freq,
        const bool optimize_power,
        hal5_flash_latency_t* latency,
        hal5_pwr_voltage_scaling_t* vos);

void hal5_flash_change_latency(
        const hal5_flash_latency_t latency);

void hal5_flash_enable_prefetch(void);

// GPIO

void hal5_gpio_configure_as_input(
        const hal5_gpio_pin_t pin,
        const hal5_gpio_mode_t mode);

void hal5_gpio_configure_as_output(
        const hal5_gpio_pin_t pin,
        const hal5_gpio_mode_t mode,
        const hal5_gpio_output_speed_t output_speed);

void hal5_gpio_configure_as_af(
        const hal5_gpio_pin_t pin,
        const hal5_gpio_mode_t mode,
        const hal5_gpio_output_speed_t output_speed,
        const hal5_gpio_af_t af);

void hal5_gpio_configure_as_analog(
        const hal5_gpio_pin_t pin,
        const hal5_gpio_mode_t mode);

void hal5_gpio_set(
        const hal5_gpio_pin_t pin);

void hal5_gpio_reset(
        const hal5_gpio_pin_t pin);

bool hal5_gpio_get(
        const hal5_gpio_pin_t pin);

void hal5_gpio_flip(
        const hal5_gpio_pin_t pin);

// callback can be NULL
// each pin number is assigned to one EXTI
// e.g. PA0 is EXTI0, PB1 is EXTI1
// because PB0 is also EXTI0
// only one callback can be given for EXTI0
// otherwise assert will fail
void hal5_gpio_configure_exti(
        const hal5_gpio_pin_t pin,
        const bool rising_edge_trigger,
        const bool falling_edge_trigger,
        void (*callback)(void));

// I2C

void hal5_i2c_configure();

bool hal5_i2c_read(
        uint8_t* ch);

void hal5_i2c_write(
        const uint8_t ch);

// LPUART

void hal5_lpuart_configure(
        const uint32_t baud);

void hal5_lpuart_dump_info();

void hal5_lpuart_write(
        const char ch);

bool hal5_lpuart_read(
        char* ch);

// PWR

void hal5_pwr_enable_usb33(void);

hal5_pwr_voltage_scaling_t hal5_pwr_get_voltage_scaling(void);

void hal5_pwr_change_voltage_scaling(
        const hal5_pwr_voltage_scaling_t vos);

// RCC
//
hal5_rcc_reset_status_t hal5_rcc_get_reset_status(void);
void hal5_rcc_dump_clock_info(void);

void hal5_rcc_change_hsidiv(
        const hal5_rcc_hsidiv_t hsidiv);

void hal5_rcc_change_lpuart1_ker_ck(
        const hal5_rcc_lpuart1sel_t src);

void hal5_rcc_enable_csi(void);
void hal5_rcc_enable_lse_bypass(void);
void hal5_rcc_enable_lse_crystal(void);
void hal5_rcc_enable_lsi(void);
void hal5_rcc_enable_hse_bypass(void);
void hal5_rcc_enable_hse_crystal(void);
void hal5_rcc_enable_hsi(void);
void hal5_rcc_enable_hsi48(void);

bool hal5_rcc_is_csi_enabled(void);
bool hal5_rcc_is_lse_enabled(void);
bool hal5_rcc_is_lsi_enabled(void);
bool hal5_rcc_is_hse_enabled(void);
bool hal5_rcc_is_hsi_enabled(void);
bool hal5_rcc_is_hsi48_enabled(void);

void hal5_rcc_enable_gpio_port_by_index(
        const uint32_t port_index);

void hal5_rcc_enable_lpuart1(void);

void hal5_rcc_enable_mco2(
        const hal5_rcc_mco2sel_t src, 
        const uint32_t prescaler);

void hal5_rcc_enable_rng(void);
void hal5_rcc_enable_usb(void);

void hal5_rcc_change_sys_ck_src(
        const hal5_rcc_sys_ck_src_t src);

void hal5_rcc_initialize_pll1(
        const hal5_rcc_pll_src_t src, 
        const uint32_t divm, 
        uint32_t muln,
        uint32_t divp, 
        uint32_t divq, 
        uint32_t divr, 
        const bool pen, 
        const bool qen, 
        const bool ren);

// RCC ck

// this method does not enable LSE
void hal5_rcc_set_lse_ck(
        const uint32_t lse_ck);

// this method does not enable HSE
void hal5_rcc_set_hse_ck(
        const uint32_t hse_ck);

uint32_t hal5_rcc_get_csi_ck(void);
uint32_t hal5_rcc_get_lse_ck(void);
uint32_t hal5_rcc_get_lsi_ck(void);
uint32_t hal5_rcc_get_hse_ck(void);
uint32_t hal5_rcc_get_hsi_ck(void);
uint32_t hal5_rcc_get_hsi48_ck(void);

uint32_t hal5_rcc_get_csi_ker_ck(void);
uint32_t hal5_rcc_get_lse_ker_ck(void);
uint32_t hal5_rcc_get_lsi_ker_ck(void);
uint32_t hal5_rcc_get_hsi_ker_ck(void);
uint32_t hal5_rcc_get_hsi48_ker_ck(void);

uint32_t hal5_rcc_get_sys_ck(void);
uint32_t hal5_rcc_get_pll1_p_ck(void);

uint32_t hal5_rcc_get_fclk(void);
uint32_t hal5_rcc_get_i2c_ker_ck(
        const uint32_t n);
uint32_t hal5_rcc_get_lpuart1_ker_ck(void);
uint32_t hal5_rcc_get_systick_ck(void);

// RNG

void hal5_rng_enable(void);
uint32_t hal5_rng_random(void);

// SYSTICK

extern volatile uint32_t hal5_ticks;
extern volatile uint32_t hal5_slow_ticks;

void hal5_systick_configure(void);
void hal5_wait(
        const uint32_t milliseconds);

// WATCHDOG

void hal5_watchdog_configure(
        const uint32_t milliseconds);
void hal5_watchdog_heartbeat(void);

#ifdef __cplusplus
}
#endif

#endif
