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

static uint32_t lse_ck = 0;
static uint32_t hse_ck = 0;

// LSE AND HSE depends in external configuration
// either a direct clock input or crystal etc.

void hal5_rcc_set_lse_ck(const uint32_t ck)
{
  lse_ck = ck;
}

void hal5_rcc_set_hse_ck(const uint32_t ck)
{
  hse_ck = ck;
}

// CORE CLOCKS
// THESE ARE THE SOURCE OF ALL OTHER CLOCKS
// HENCE THEY HAVE FIXED VALUES

uint32_t hal5_rcc_get_csi_ck()
{
  return 4000000;
}

uint32_t hal5_rcc_get_lse_ck()
{
  return lse_ck;
}

uint32_t hal5_rcc_get_lsi_ck()
{
  return 32000;
}

uint32_t hal5_rcc_get_hse_ck()
{
  return hse_ck;
}

// hsi prescaler
static uint32_t hal5_rcc_get_hsidiv() 
{
  const uint32_t hsidiv = ((RCC->CR & RCC_CR_HSIDIV_Msk) >> RCC_CR_HSIDIV_Pos);
  
  if (hsidiv == 0b00) return 1;
  else if (hsidiv == 0b01) return 2;
  else if (hsidiv == 0b10) return 4;
  else if (hsidiv == 0b11) return 8;
  else assert (false);
}

// there is no way to get hsi_ck alone
// there is always a prescaler in front of it
uint32_t hal5_rcc_get_hsi_ck()
{
  return 64000000 / hal5_rcc_get_hsidiv();
}

uint32_t hal5_rcc_get_hsi48_ck()
{
  return 48000000;
}

// KERNEL CLOCKS DERIVED FROM CORE CLOCKS

// csi_ker_ck has no prescaler
uint32_t hal5_rcc_get_csi_ker_ck()
{
  return hal5_rcc_get_csi_ck();
}

// lse_ker_ck has no prescaler
uint32_t hal5_rcc_get_lse_ker_ck()
{
  return hal5_rcc_get_lse_ck();
}

// lsi_ker_ck has no prescaler
uint32_t hal5_rcc_get_lsi_ker_ck()
{
  return hal5_rcc_get_lsi_ck();
}

// hsi_ker_ck has a prescaler
uint32_t hal5_rcc_get_hsi_ker_ck()
{
  return hal5_rcc_get_hsi_ck();
}

// hsi48_ker_ck has no prescaler
uint32_t hal5_rcc_get_hsi48_ker_ck()
{
  return hal5_rcc_get_hsi_ck();
}

// ALL DERIVED CLOCKS BELOW
// DERIVED WITH DIVISORS/PRESCALERS/PLLS FROM ABOVE

static uint32_t hal5_rcc_get_hpre() 
{
  const uint32_t hpre = ((RCC->CFGR2 & RCC_CFGR2_HPRE_Msk) 
      >> RCC_CFGR2_HPRE_Pos);

  switch (hpre)
  {
    case 0b0000:
    case 0b0001:
    case 0b0010:
    case 0b0011:
    case 0b0100:
    case 0b0101:
    case 0b0110:
    case 0b0111: return 1;
    case 0b1000: return 2;
    case 0b1001: return 4;
    case 0b1010: return 8;
    case 0b1011: return 16;
    // value for 32
    case 0b1100: return 64;
    case 0b1101: return 128;
    case 0b1110: return 256;
    case 0b1111: return 512;
    default: assert (false);
  }
}

static uint32_t hal5_rcc_get_ppre(uint32_t n) 
{
  assert (n > 0);
  assert (n <= 3);

  const uint32_t pos  = 4 + (4 * (n-1));
  const uint32_t mask = 0x7 << pos;
  const uint32_t ppre = ((RCC->CFGR2 & mask) >> pos);

  switch (ppre)
  {
    case 0b000:
    case 0b001:
    case 0b010:
    case 0b011: return 1;
    case 0b100: return 2;
    case 0b101: return 4;
    case 0b110: return 8;
    case 0b111: return 16;
    default: assert (false);
  }
}

static uint32_t hal5_rcc_get_ppre1() 
{
  return hal5_rcc_get_ppre(1);
}

static uint32_t hal5_rcc_get_ppre2() 
{
  return hal5_rcc_get_ppre(2);
}

static uint32_t hal5_rcc_get_ppre3() 
{
  return hal5_rcc_get_ppre(3);
}

static uint32_t hal5_rcc_get_pll1_input_ck()
{
  const uint32_t v = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1SRC_Msk)
      >> RCC_PLL1CFGR_PLL1SRC_Pos);

  switch (v)
  {
    case 0b00: return 0;
    case 0b01: return hal5_rcc_get_hsi_ck();
    case 0b10: return hal5_rcc_get_csi_ck();
    case 0b11: return hal5_rcc_get_hse_ck();
    default: assert (false);
  }
}

static uint32_t hal5_rcc_get_pll1_m()
{
  const uint32_t v = ((RCC->PLL1CFGR & RCC_PLL1CFGR_PLL1M_Msk)
      >> RCC_PLL1CFGR_PLL1M_Pos);
  // v=0 means disabled
  if (v == 0) return 0xFFFFFFFF;
  else return v;
}

// register is encoded as one less
static uint32_t hal5_rcc_get_pll1_n()
{
  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1N_Msk)
      >> RCC_PLL1DIVR_PLL1N_Pos)+1;
}

// register is encoded as one less
static uint32_t hal5_rcc_get_pll1_p()
{
  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1P_Msk)
      >> RCC_PLL1DIVR_PLL1P_Pos)+1;
}

// register is encoded as one less
static uint32_t hal5_rcc_get_pll1_q()
{
  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1Q_Msk)
      >> RCC_PLL1DIVR_PLL1Q_Pos)+1;
}

// register is encoded as one less
static uint32_t hal5_rcc_get_pll1_r()
{
  return ((RCC->PLL1DIVR & RCC_PLL1DIVR_PLL1R_Msk)
      >> RCC_PLL1DIVR_PLL1R_Pos)+1;
}

uint32_t hal5_rcc_get_pll1_p_ck()
{
  return (((hal5_rcc_get_pll1_input_ck() / hal5_rcc_get_pll1_m())
      * hal5_rcc_get_pll1_n())
      / hal5_rcc_get_pll1_p());
}

static uint32_t hal5_rcc_get_pll1_q_ck() { assert(false); }
static uint32_t hal5_rcc_get_pll1_r_ck() { assert(false); }

static uint32_t hal5_rcc_get_pll2_p_ck() { assert(false); }
static uint32_t hal5_rcc_get_pll2_q_ck() { assert(false); }
static uint32_t hal5_rcc_get_pll2_r_ck() { assert(false); }

static uint32_t hal5_rcc_get_pll3_p_ck() { assert(false); }
static uint32_t hal5_rcc_get_pll3_q_ck() { assert(false); }
static uint32_t hal5_rcc_get_pll3_r_ck() { assert(false); }

uint32_t hal5_rcc_get_sys_ck()
{
  const uint32_t v = ((RCC->CFGR1 & RCC_CFGR1_SWS_Msk) 
      >> RCC_CFGR1_SWS_Pos);

  switch (v) 
  {
    case 0b00: return hal5_rcc_get_hsi_ck();
    case 0b01: return hal5_rcc_get_csi_ck();
    case 0b10: return hal5_rcc_get_hse_ck();
    case 0b11: return hal5_rcc_get_pll1_p_ck();
    default: assert (false);
  }
}

// alias for sys_ck
static uint32_t hal5_rcc_get_sysclk()
{
  return hal5_rcc_get_sys_ck();
}

static uint32_t hal5_rcc_get_rcc_hclk() 
{
  return (hal5_rcc_get_sys_ck() / hal5_rcc_get_hpre());
}

// alias
static uint32_t hal5_rcc_get_hclk() 
{
  return hal5_rcc_get_rcc_hclk();
}

static uint32_t hal5_rcc_get_rcc_pclk1() 
{
  return (hal5_rcc_get_hclk() / hal5_rcc_get_ppre1());
} 

// alias
static uint32_t hal5_rcc_get_pclk1() 
{
  return hal5_rcc_get_rcc_pclk1();
}

static uint32_t hal5_rcc_get_rcc_pclk2() 
{
  return (hal5_rcc_get_hclk() / hal5_rcc_get_ppre2());
} 

// alias
static uint32_t hal5_rcc_get_pclk2() 
{
  return hal5_rcc_get_rcc_pclk2();
}

static uint32_t hal5_rcc_get_rcc_pclk3() 
{
  return (hal5_rcc_get_hclk() / hal5_rcc_get_ppre3());
} 

// alias
static uint32_t hal5_rcc_get_pclk3() 
{
  return hal5_rcc_get_rcc_pclk3();
}

uint32_t hal5_rcc_get_fclk()
{
  return hal5_rcc_get_hclk();
}

uint32_t hal5_rcc_get_systick_ck()
{
  if (SysTick->CTRL & 0x4)
  {
    return hal5_rcc_get_fclk();
  }
  else
  {
    const uint32_t v = (RCC->CCIPR4 & RCC_CCIPR4_SYSTICKSEL_Msk)
      >> RCC_CCIPR4_SYSTICKSEL_Pos;

    switch (v)
    {
      case 0b00: return (hal5_rcc_get_hclk() / 8);
      case 0b01: return hal5_rcc_get_lsi_ker_ck();
      case 0b10: return hal5_rcc_get_lse_ck();
      default: assert (false);
    }
  }
}

uint32_t hal5_rcc_get_lpuart1_ker_ck()
{
  const uint32_t v = (RCC->CCIPR3 & RCC_CCIPR3_LPUART1SEL_Msk)
      >> RCC_CCIPR3_LPUART1SEL_Pos;

  switch (v)
  {
    case 0b000: return hal5_rcc_get_pclk3();
    case 0b001: return hal5_rcc_get_pll2_q_ck();
    case 0b010: return hal5_rcc_get_pll3_q_ck();
    case 0b011: return hal5_rcc_get_hsi_ker_ck();
    case 0b100: return hal5_rcc_get_csi_ker_ck();
    case 0b101: return hal5_rcc_get_lse_ker_ck();
    default: return 0;
  }
}

uint32_t hal5_rcc_get_i2c_ker_ck(uint32_t n)
{
  assert (n > 0);
  assert (n <= 4);

  uint32_t pos  = 16 + (2*(n-1));
  uint32_t mask = 0x3 << pos;

  const uint32_t v = (RCC->CCIPR4 & mask) >> pos;

  switch (v)
  {
    case 0b00: 
    {
      if (n < 3) 
      {
        return hal5_rcc_get_pclk1();
      } 
      else
      {
        return hal5_rcc_get_pclk3();
      }
    }
    case 0b01: return hal5_rcc_get_pll3_r_ck();
    case 0b10: return hal5_rcc_get_hsi_ker_ck();
    case 0b11: return hal5_rcc_get_csi_ker_ck();
    default: assert (false);
  }

}

void hal5_rcc_dump_clock_info(void)
{
  const uint32_t K = 1000;
  const uint32_t M = 1000000;

  CONSOLE("CSI     : %3lu MHz\n", hal5_rcc_get_csi_ck() / M);
  CONSOLE("LSI     : %3lu KHz\n", hal5_rcc_get_lsi_ck() / K);
  CONSOLE("HSI     : %3lu MHz\n", hal5_rcc_get_hsi_ck() / M);
  CONSOLE("PLL1_P  : %3lu MHz\n", hal5_rcc_get_pll1_p_ck() / M);
  CONSOLE("SYSCLK  : %3lu MHz\n", hal5_rcc_get_sys_ck() / M);
  CONSOLE("HCLK    : %3lu MHz\n", hal5_rcc_get_hclk() / M);
  CONSOLE("FCLK    : %3lu MHz\n", hal5_rcc_get_fclk() / M);
}
