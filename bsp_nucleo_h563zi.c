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

#include "bsp.h"
#include "hal5.h"

#define RED_LED     PG4
#define YELLOW_LED  PF4
#define GREEN_LED   PB0
#define USER_BUTTON PC13

void bsp_configure(
    void (*button_callback)(void))
{
  // NUCLEO H563ZI board default configuraiton
  // LSE=32768 X2 Crystal Oscillator
  hal5_rcc_set_lse_ck(32768);
  // HSE=8MHz input from STLINK-V3EC
  hal5_rcc_set_hse_ck(8000000);

  // RED PG4
  hal5_gpio_configure_as_output(
      RED_LED,
      output_pp_floating,
      low_speed);

  // YELLOW PF4
  hal5_gpio_configure_as_output(
      YELLOW_LED,
      output_pp_floating,
      low_speed);

  // GREEN PB0
  hal5_gpio_configure_as_output(
      GREEN_LED,
      output_pp_floating,
      low_speed);

  // USER BUTTON PC13
  hal5_gpio_configure_as_input(
      USER_BUTTON,
      input_floating);

  hal5_gpio_configure_exti(
      USER_BUTTON, 
      true, 
      false,
      button_callback);
}

void bsp_show(uint8_t code) 
{
  assert (code <= 0x7);

  if (code & 0x4) {
    hal5_gpio_set(RED_LED);
  } else {
    hal5_gpio_reset(RED_LED);
  }

  if (code & 0x2) {
    hal5_gpio_set(YELLOW_LED);
  } else {
    hal5_gpio_reset(YELLOW_LED);
  }

  if (code & 0x1) {
    hal5_gpio_set(GREEN_LED);
  } else {
    hal5_gpio_reset(GREEN_LED);
  }
}

void bsp_boot_completed(void)
{
  hal5_gpio_reset(RED_LED);
  hal5_gpio_reset(YELLOW_LED);
  hal5_gpio_reset(GREEN_LED);
} 

void bsp_fault(void)
{
  hal5_gpio_set(RED_LED);
  hal5_gpio_reset(YELLOW_LED);
  hal5_gpio_reset(GREEN_LED);
}

void bsp_heartbeat(void)
{
  hal5_gpio_flip(GREEN_LED);
}
