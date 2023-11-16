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

#include "mcu.h"

void mcu_configure_trace()
{
  /*
  // enable GPIO port E clock
	RCC->AHB2ENR |= 0b10000;

  // PE2..PE6 AF0
  mcu_configure_gpio(GPIOE, 2, 0b10, 0, 0b11, 0b01, 0b0000);
  mcu_configure_gpio(GPIOE, 3, 0b10, 0, 0b11, 0b01, 0b0000);
  mcu_configure_gpio(GPIOE, 4, 0b10, 0, 0b11, 0b01, 0b0000);
  mcu_configure_gpio(GPIOE, 5, 0b10, 0, 0b11, 0b01, 0b0000);
  mcu_configure_gpio(GPIOE, 6, 0b10, 0, 0b11, 0b01, 0b0000);
  */

  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

  MODIFY_REG(DBGMCU->CR, DBGMCU_CR_TRACE_MODE_Msk, 
      0b11 << DBGMCU_CR_TRACE_MODE_Pos);

  MODIFY_REG(DBGMCU->CR, DBGMCU_CR_TRACE_CLKEN_Msk, 
      DBGMCU_CR_TRACE_CLKEN);

  MODIFY_REG(DBGMCU->CR, DBGMCU_CR_TRACE_IOEN_Msk, 
      DBGMCU_CR_TRACE_IOEN);

  //TPI->SPPR = 0;      // parallel trace mode (sync)
  //TPI->CSPSR = (1<<3);// port size=4
  //TPI->ACPR = 0;      // no prescaler = /(0+1)=/1
 
  /*
  DWT->CTRL |= (2 << DWT_CTRL_SYNCTAP_Pos) |
               (1 << DWT_CTRL_CYCCNTENA_Pos);
  */

  // configure ITM
  ITM->LAR = 0xC5ACCE55;
  ITM->TER = 0xFFFFFFFF;
  MODIFY_REG(ITM->TCR, ITM_TCR_TRACEBUSID_Msk,
      (1 << ITM_TCR_TRACEBUSID_Pos));
  MODIFY_REG(ITM->TCR, ITM_TCR_DWTENA_Msk,
      (0 << ITM_TCR_DWTENA_Pos));
  MODIFY_REG(ITM->TCR, ITM_TCR_SYNCENA_Msk,
      (0 << ITM_TCR_SYNCENA_Pos));
  MODIFY_REG(ITM->TCR, ITM_TCR_TSENA_Msk,
      (0 << ITM_TCR_TSENA_Pos));
  MODIFY_REG(ITM->TCR, ITM_TCR_ITMENA_Msk,
      (1 << ITM_TCR_ITMENA_Pos));
  

  /*
  // configure ETM
  ETM->LAR = 0xC5ACCE55;
  ETM->CR = ETM_CR_ETMEN |
            ETM_CR_STALL_PROCESSOR |
            ETM_CR_BRANCH_OUTPUT;
  ETM->TRACEIDR = 2;
  ETM->TECR1 = ETM_TECR1_EXCLUDE;
  ETM->FFRR = ETM_FFRR_EXCLUDE;
  ETM->FFLR = 24;
  */

}
