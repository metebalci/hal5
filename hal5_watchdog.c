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

void hal5_watchdog_configure(
        const uint32_t milliseconds)
{
  assert (milliseconds <= 0x3FFF);

  // enable IWDG
  IWDG->KR = 0xCCCC;
  // enable registere access
  IWDG->KR = 0x5555;
  // prescaler /128, LSI is 32kHz
  // this makes iwdwg counter 250Hz, 4ms
  IWDG->PR = 0b0101;
  // reload value, 12 bit
  // 3E8=1000, 1second with 1kHz counting
  IWDG->RLR = (milliseconds >> 2);
  // wait for reload value and prescaler registers to be updated
  while ((IWDG->SR & 0b11) != 0);
  // refresh and write protect registers
  IWDG->KR = 0xAAAA;
}

void hal5_watchdog_heartbeat(void)
{
  IWDG->KR = 0xAAAA;
}
