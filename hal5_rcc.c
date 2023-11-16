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

hal5_rcc_reset_status_t hal5_rcc_get_reset_status(void)
{
  const uint32_t rsr = RCC->RSR;

  if (rsr & RCC_RSR_LPWRRSTF) {
    return reset_status_illegal_stop_entry;
  } else if (rsr & RCC_RSR_WWDGRSTF) {
    return reset_status_window_watchdog;
  } else if (rsr & RCC_RSR_IWDGRSTF) {
    return reset_status_independent_watchdog;
  } else if (rsr & RCC_RSR_SFTRSTF) {
    return reset_status_system_reset_by_cpu;
  } else if (rsr & RCC_RSR_BORRSTF) {
    return reset_status_bor;
  } else if (rsr & RCC_RSR_PINRSTF) {
    return reset_status_pin;
  } else {
    return reset_status_unknown;
  }

  SET_BIT(RCC->RSR, RCC_RSR_RMVF);
}

void hal5_rcc_change_hsidiv(hal5_rcc_hsidiv_t hsidiv)
{
  uint32_t hsidiv_bits;

  switch (hsidiv)
  {
    case hsidiv_1: hsidiv_bits = 0b00; break;
    case hsidiv_2: hsidiv_bits = 0b01; break;
    case hsidiv_4: hsidiv_bits = 0b10; break;
    case hsidiv_8: hsidiv_bits = 0b11; break;
    default: assert (false);
  }

  MODIFY_REG(RCC->CR, RCC_CR_HSIDIV_Msk,
      hsidiv_bits << RCC_CR_HSIDIV_Pos);

  while ((RCC->CR & RCC_CR_HSIDIVF) == 0);
}

bool hal5_rcc_is_csi_enabled()
{
  return ((RCC->CR & RCC_CR_CSIRDY) != 0);
}

bool hal5_rcc_is_lse_enabled()
{
  return ((RCC->BDCR & RCC_BDCR_LSERDY) != 0);
}

bool hal5_rcc_is_lsi_enabled()
{
  return ((RCC->BDCR & RCC_BDCR_LSIRDY) != 0);
}

bool hal5_rcc_is_hse_enabled()
{
  return ((RCC->CR & RCC_CR_HSERDY) != 0);
}

bool hal5_rcc_is_hsi_enabled()
{
  return ((RCC->CR & RCC_CR_HSIRDY) != 0);
}

bool hal5_rcc_is_hsi48_enabled()
{
  return ((RCC->CR & RCC_CR_HSI48RDY) != 0);
}

void hal5_rcc_enable_csi(void)
{
  if (!hal5_rcc_is_csi_enabled()) {
    SET_BIT(RCC->CR, RCC_CR_CSION);
    // wait until ready
    while ((RCC->CR & RCC_CR_CSIRDY) == 0);
  }
}

void hal5_rcc_enable_lse_bypass(void) { assert(false); }
void hal5_rcc_enable_lse_crystal(void) { assert(false); }

void hal5_rcc_enable_lsi(void) { assert(false); }

void hal5_rcc_enable_hsi(void) 
{
  if (!hal5_rcc_is_hsi_enabled())
  {
    SET_BIT(RCC->CR, RCC_CR_HSION);
    while ((RCC->CR & RCC_CR_HSIRDY) == 0);
  }
}

void hal5_rcc_enable_hse_bypass(void) { assert(false); }
void hal5_rcc_enable_hse_crystal(void) { assert(false); }

void hal5_rcc_enable_hsi48(void) 
{
  if (!hal5_rcc_is_hsi48_enabled())
  {
    SET_BIT(RCC->CR, RCC_CR_HSI48ON);
    while ((RCC->CR & RCC_CR_HSI48RDY) == 0);
  }
}

void hal5_rcc_enable_gpio_port_by_index(uint32_t port_index)
{
  SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_GPIOAEN << port_index);
}

void hal5_rcc_enable_lpuart1() {
  SET_BIT(RCC->APB3ENR, RCC_APB3ENR_LPUART1EN);
}

void hal5_rcc_enable_rng() {
	SET_BIT(RCC->AHB2ENR, RCC_AHB2ENR_RNGEN);
}

void hal5_rcc_enable_usb() 
{
  SET_BIT(RCC->APB2ENR, RCC_APB2ENR_USBEN);
}

void hal5_rcc_enable_mco2(
    hal5_rcc_mco2sel_t src,
    uint32_t prescaler)
{
  assert (prescaler <= 0xF);

  uint32_t mco2sel_bits;

  switch (src)
  {
    case mco2sel_sysclk:  mco2sel_bits=0b0000; break;
    case mco2sel_pll2:    mco2sel_bits=0b0001; break;
    case mco2sel_hse:     mco2sel_bits=0b0010; break;
    case mco2sel_pll1:    mco2sel_bits=0b0011; break;
    case mco2sel_csi:     mco2sel_bits=0b0100; break;
    case mco2sel_lsi:     mco2sel_bits=0b0101; break;
    default: assert (false);
  }

  MODIFY_REG(RCC->CFGR1, RCC_CFGR1_MCO2SEL_Msk,
      mco2sel_bits << RCC_CFGR1_MCO2SEL_Pos);

  MODIFY_REG(RCC->CFGR1, RCC_CFGR1_MCO2PRE_Msk,
      prescaler << RCC_CFGR1_MCO2PRE_Pos);

  // MCO2 is PC9 AF0
  hal5_gpio_configure_as_af(
      PC9,
      af_pp_floating,
      very_high_speed,
      AF0);
}

void hal5_rcc_change_sys_ck_src(hal5_rcc_sys_ck_src_t src)
{ 
  uint32_t src_bits;

  switch (src)
  {
    case sys_ck_src_hsi: src_bits=0b00; break;
    case sys_ck_src_csi: src_bits=0b01; break;
    case sys_ck_src_hse: src_bits=0b10; break;
    case sys_ck_src_pll1: src_bits=0b11; break;
    default: assert (false);
  }

  MODIFY_REG(RCC->CFGR1, RCC_CFGR1_SW_Msk,
      src_bits << RCC_CFGR1_SW_Pos);

  while (((RCC->CFGR1 & RCC_CFGR1_SWS_Msk) >> RCC_CFGR1_SWS_Pos) != src_bits);
}

// initialize PLL1 integer mode
void hal5_rcc_initialize_pll1(
    const hal5_rcc_pll_src_t src,
    const uint32_t divm, 
    uint32_t muln,
    uint32_t divp, 
    uint32_t divq, 
    uint32_t divr,
    const bool pen, 
    const bool qen, 
    const bool ren)
{
  assert (divm <= 0x3F);

  // 4 <= divn <= 512
  assert (muln >= 4);
  assert (muln <= 512);

  // divp odd factors not allowed, and <= 128
  assert (divp % 2 == 0);
  assert (divp <= 128);

  // 1 <= divq <= 128
  assert (divq > 0);
  assert (divq <= 128);
  
  // 1 <= divr <= 128
  assert (divr > 0);
  assert (divr <= 128);

  // muln=4 is encoded as 3
  muln--;  
  // divp=2 is encoded as 1
  divp--; 
  // divq=1 is encoded as 0
  divq--;
  // divr=1 is encoded as 0
  divr--;

  // ref: RM0481
  // Figure 52. PLLs initialization flow

  // set PLL src
  uint32_t src_bits;
  switch (src)
  {
    case pll_src_hsi: src_bits = 0b01; break;
    case pll_src_csi: src_bits = 0b10; break;
    case pll_src_hse: src_bits = 0b11; break;
    default: assert (false);
  }

  MODIFY_REG(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1SRC_Msk,
    src_bits << RCC_PLL1CFGR_PLL1SRC_Pos);

  MODIFY_REG(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1M_Msk,
      divm << RCC_PLL1CFGR_PLL1M_Pos);

  uint32_t input_ck;
  if (src == pll_src_hsi) 
  {
    assert (hal5_rcc_is_hsi_enabled());
    input_ck = hal5_rcc_get_hsi_ck();
  }
  else if (src == pll_src_csi) 
  {
    assert (hal5_rcc_is_csi_enabled());
    input_ck = hal5_rcc_get_csi_ck();
  }
  else if (src == pll_src_hse) 
  {
    assert (hal5_rcc_is_hse_enabled());
    input_ck = hal5_rcc_get_hse_ck();
  }
  else assert (false);
  input_ck = input_ck / divm;

  const uint32_t vco_output = input_ck * muln;
  if ((vco_output >= 150000000) && (vco_output <= 420000000)) 
  {
    SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1VCOSEL);
  }
  else if ((vco_output >= 192000000) && (vco_output <= 836000000)) 
  {
    CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1VCOSEL);
  }
  else 
  {
    assert (false);
  }

  uint32_t rge;
  if (input_ck <= 2000000) 
  {
    rge = 0b00;
  }
  else if (input_ck <= 4000000) 
  {
    rge = 0b01;
  }
  else if (input_ck <= 8000000) 
  {
    rge = 0b10;
  }
  else if (input_ck <= 16000000) 
  {
    rge = 0b11;
  }
  else 
  {
    assert (false);
  }

  // set PLL input freq. range
  MODIFY_REG(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1RGE_Msk,
      rge << RCC_PLL1CFGR_PLL1RGE_Pos);

  // disable fractional part
  CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1FRACEN);

  // enable/disable P output
  if (pen) SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1PEN);
  else CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1PEN);

  // enable/disable Q output
  if (qen) SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1QEN);
  else CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1QEN);

  // enable/disable R output
  if (ren) SET_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1REN);
  else CLEAR_BIT(RCC->PLL1CFGR, RCC_PLL1CFGR_PLL1REN);

  // set div N, P, Q, R
  RCC->PLL1DIVR = (RCC->PLL1DIVR & 0x80800000) |
    ((divr << RCC_PLL1DIVR_PLL1R_Pos) |
     (divq << RCC_PLL1DIVR_PLL1Q_Pos) |
     (divp << RCC_PLL1DIVR_PLL1P_Pos) |
     (muln << RCC_PLL1DIVR_PLL1N_Pos));

  // enable PLL
  SET_BIT(RCC->CR, RCC_CR_PLL1ON);

  // wait for PLL to lock
  while ((RCC->CR & RCC_CR_PLL1RDY_Msk) == 0);
}

void hal5_rcc_change_lpuart1_ker_ck(hal5_rcc_lpuart1sel_t src)
{
  uint32_t src_bits;

  switch (src)
  {
    case lpuart1sel_pclk3:      src_bits=0b000; break;
    case lpuart1sel_pll2_q_ck:  src_bits=0b001; break;
    case lpuart1sel_pll3_q_ck:  src_bits=0b010; break;
    case lpuart1sel_hsi_ker_ck: src_bits=0b011; break;
    case lpuart1sel_csi_ker_ck: src_bits=0b100; break;
    case lpuart1sel_lse_ck:     src_bits=0b101; break;
    default: assert (false);
  }

  MODIFY_REG(RCC->CCIPR3, RCC_CCIPR3_LPUART1SEL_Msk,
      src_bits << RCC_CCIPR3_LPUART1SEL_Pos);
}
