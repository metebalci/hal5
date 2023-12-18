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

#ifndef __HAL5_TYPES_H__
#define __HAL5_TYPES_H__

#ifdef __cplusplus
extern "C" {
#endif

// stack frame when hardfault is handled
// see main.c and startup_stm32h5xx.c for example
// pc contains where fault happened
// lr contains 
typedef __PACKED_STRUCT
{
    uint32_t r0;
    uint32_t r1;
    uint32_t r2;
    uint32_t r3;
    uint32_t r12;
    uint32_t lr;
    uint32_t pc;
} hal5_exception_stack_frame_t;


// FLASH
typedef enum
{
  hal5_flash_0ws,
  hal5_flash_1ws,
  hal5_flash_2ws,
  hal5_flash_3ws,
  hal5_flash_4ws,
  hal5_flash_5ws,
} hal5_flash_latency_t;

// GPIO

#define MAKE_GPIO_PIN(x, y) (((x-'A')<<8) | y)

typedef enum
{
  // signal for debugging
  PA0   = MAKE_GPIO_PIN('A', 0),

  // USB
  PA11  = MAKE_GPIO_PIN('A', 11),
  PA12  = MAKE_GPIO_PIN('A', 12),

  // BSP LED
  PB0   = MAKE_GPIO_PIN('B', 0),

  // LPUART1
  PB6   = MAKE_GPIO_PIN('B', 6),
  PB7   = MAKE_GPIO_PIN('B', 7),

  // MCO2
  PC9   = MAKE_GPIO_PIN('C', 9),

  // BSP BUTTON
  PC13  = MAKE_GPIO_PIN('C', 13),

  // BSP LED
  PG4   = MAKE_GPIO_PIN('G', 4),

  // I2C2
  PF0   = MAKE_GPIO_PIN('F', 0),
  PF1   = MAKE_GPIO_PIN('F', 1),

  // BSP LED
  PF4   = MAKE_GPIO_PIN('F', 4),
} hal5_gpio_pin_t;

typedef enum
{
  input_floating,
  input_pull_up,
  input_pull_down,
  output_od_floating,
  output_od_pull_up,
  output_od_pull_down,
  output_pp_floating,
  output_pp_pull_up,
  output_pp_pull_down,
  af_od_floating,
  af_od_pull_up,
  af_od_pull_down,
  af_pp_floating,
  af_pp_pull_up,
  af_pp_pull_down,
  analog
} hal5_gpio_mode_t;

typedef enum 
{
  AF0, AF1, AF2, AF3, AF4, AF5, AF6, AF7, AF8, AF9,
  AF10, AF11, AF12, AF13, AF14, AF15,
  // dont care is used internally
  hal5_gpio_af_dont_care
} hal5_gpio_af_t;

typedef enum
{
  // dont care is used internally
  hal5_gpio_output_speed_dont_care,
  low_speed,
  medium_speed,
  high_speed,
  very_high_speed
} hal5_gpio_output_speed_t;

// HASH

typedef enum
{
    hal5_hash_reserved,
    hal5_hash_sha1,
    hal5_hash_sha2_224,
    hal5_hash_sha2_256,
    hal5_hash_sha2_384,
    hal5_hash_sha2_512_224,
    hal5_hash_sha2_512_256,
    hal5_hash_sha2_512,
} hal5_hash_algorithm_t;

// PWR

typedef enum 
{
  hal5_pwr_vos3,
  hal5_pwr_vos2,
  hal5_pwr_vos1,
  hal5_pwr_vos0
} hal5_pwr_voltage_scaling_t;

// RCC

typedef enum
{
  hsidiv_1,
  hsidiv_2,
  hsidiv_4,
  hsidiv_8
} hal5_rcc_hsidiv_t;

typedef enum
{
  mco2sel_sysclk,
  mco2sel_pll2,
  mco2sel_hse,
  mco2sel_pll1,
  mco2sel_csi,
  mco2sel_lsi
} hal5_rcc_mco2sel_t;

typedef enum
{
  pll_src_hsi,
  pll_src_csi,
  pll_src_hse
} hal5_rcc_pll_src_t;

typedef enum
{
  sys_ck_src_hsi,
  sys_ck_src_csi,
  sys_ck_src_hse,
  sys_ck_src_pll1
} hal5_rcc_sys_ck_src_t;

typedef enum
{
  lpuart1sel_pclk3,
  lpuart1sel_pll2_q_ck,
  lpuart1sel_pll3_q_ck,
  lpuart1sel_hsi_ker_ck,
  lpuart1sel_csi_ker_ck,
  lpuart1sel_lse_ck
} hal5_rcc_lpuart1sel_t;

typedef enum
{
  reset_status_unknown,
  reset_status_illegal_stop_entry,
  reset_status_window_watchdog,
  reset_status_independent_watchdog,
  reset_status_system_reset_by_cpu,
  reset_status_bor,
  reset_status_pin
} hal5_rcc_reset_status_t;

#ifdef __cplusplus
}
#endif

#endif
