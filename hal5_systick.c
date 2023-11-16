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

// not using the SysTick_Config in CMSIS

volatile uint32_t hal5_ticks = 0;
volatile uint32_t hal5_slow_ticks = 0;

static volatile uint32_t slowticksdiv = 0;
static volatile uint32_t tick_timer = 0;

void SysTick_Handler(void)
{
  hal5_ticks++;
  slowticksdiv++;
  if (slowticksdiv == 1000) {
    slowticksdiv = 0;
    hal5_slow_ticks++;
  }
  if (tick_timer > 0) tick_timer--;
}

void hal5_systick_configure()
{
  // must change CTRL first to set SysTick clock source 
  // mcc_get_systick_ck depends on it
  SysTick->CTRL = 0b111;

  const uint32_t systick_ck = hal5_rcc_get_systick_ck();

  // make sure an exact systick can be configured
  assert (systick_ck % 1000 == 0);
  // make sure an exact slow systick can be configured
  assert ((systick_ck / 1000) >= 1000);
  // make sure the reload value fits to 24-bits
  assert ((systick_ck / 1000) <= 0xFFFFFF);

  SysTick->LOAD = systick_ck / 1000;
  SysTick->VAL = 0;
}

// this is here to not export tick_timer symbol
void hal5_wait(uint32_t milliseconds)
{
  tick_timer = milliseconds;
  while (tick_timer > 0);
}
