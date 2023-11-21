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

//
// This file is based on CMSIS_5.9.0/Device/ARM/ARMCM33/Source/startup_ARMCM33.c
//
// Modifications:
// 
// - TZ/DSP/FP/CMSE support removed
// - IRQHandlers of STM32H5 series are added based reference manuals
// - a weak SystemInit symbol is defined, so it is optional
// - FPU access enabled if FPU exists
// - unaligned accesses are disabled
// 

#include <stdint.h>

#include <stm32h5xx.h>

// various macros here are defined in CMSIS 
// __INITIAL_SP is __StackTop which is defined in startup.ld
// __STACK_LIMIT is __StackLimit which is defined in startup.ld
// __PROGRAM_START is __cmsis_start which is defined in arm_cm33.h

extern uint32_t __INITIAL_SP;
extern uint32_t __STACK_LIMIT;
extern __NO_RETURN void __PROGRAM_START(void);

void SystemInit(void) __WEAK;
void HardFault_Callback(const void* stack_pointer) __WEAK;

void Default_Handler(void);
__NO_RETURN void Reset_Handler(void);

#define __WEAK_WITH_DEFAULT __attribute__ ((weak, alias("Default_Handler")))

void NMI_Handler            (void) __WEAK_WITH_DEFAULT;
void HardFault_Handler      (void) __attribute__((naked));
void MemManage_Handler      (void) __WEAK_WITH_DEFAULT;
void BusFault_Handler       (void) __WEAK_WITH_DEFAULT;
void UsageFault_Handler     (void) __WEAK_WITH_DEFAULT;
void SecureFault_Handler    (void) __WEAK_WITH_DEFAULT;
void SVC_Handler            (void) __WEAK_WITH_DEFAULT;
void DebugMon_Handler       (void) __WEAK_WITH_DEFAULT;
void PendSV_Handler         (void) __WEAK_WITH_DEFAULT;
void SysTick_Handler        (void) __WEAK_WITH_DEFAULT;
// the IRQHandler list below is created from IRQn enum typedef in stm32h563xx.h in cmsis_device_h5
void WWDG_IRQHandler (void) __WEAK_WITH_DEFAULT;
void PVD_AVD_IRQHandler (void) __WEAK_WITH_DEFAULT;
void RTC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void RTC_S_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TAMP_IRQHandler (void) __WEAK_WITH_DEFAULT;
void RAMCFG_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FLASH_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FLASH_S_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GTZC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void RCC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void RCC_S_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI0_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI7_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI8_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI9_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI10_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI11_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI12_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI13_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI14_IRQHandler (void) __WEAK_WITH_DEFAULT;
void EXTI15_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel0_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA1_Channel7_IRQHandler (void) __WEAK_WITH_DEFAULT;
void IWDG_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SAES_IRQHandler (void) __WEAK_WITH_DEFAULT;
void ADC1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void DAC1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FDCAN1_IT0_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FDCAN1_IT1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM1_BRK_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM1_UP_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM1_TRG_COM_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM1_CC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM7_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C1_EV_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C1_ER_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C2_EV_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C2_ER_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SPI1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SPI2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SPI3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USART1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USART2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USART3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UART4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UART5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPUART1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPTIM1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM8_BRK_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM8_UP_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM8_TRG_COM_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM8_CC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void ADC2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPTIM2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM15_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM16_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM17_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USB_DRD_FS_IRQHandler (void) __WEAK_WITH_DEFAULT;
void CRS_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UCPD1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FMC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void OCTOSPI1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SDMMC1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C3_EV_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C3_ER_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SPI4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SPI5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SPI6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USART6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USART10_IRQHandler (void) __WEAK_WITH_DEFAULT;
void USART11_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SAI1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SAI2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel0_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void GPDMA2_Channel7_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UART7_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UART8_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UART9_IRQHandler (void) __WEAK_WITH_DEFAULT;
void UART12_IRQHandler (void) __WEAK_WITH_DEFAULT;
void SDMMC2_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FPU_IRQHandler (void) __WEAK_WITH_DEFAULT;
void ICACHE_IRQHandler (void) __WEAK_WITH_DEFAULT;
void DCACHE1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void ETH_IRQHandler (void) __WEAK_WITH_DEFAULT;
void ETH_WKUP_IRQHandler (void) __WEAK_WITH_DEFAULT;
void DCMI_PSSI_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FDCAN2_IT0_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FDCAN2_IT1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void CORDIC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void FMAC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void DTS_IRQHandler (void) __WEAK_WITH_DEFAULT;
void RNG_IRQHandler (void) __WEAK_WITH_DEFAULT;
void OTFDEC1_IRQHandler (void) __WEAK_WITH_DEFAULT;
void AES_IRQHandler (void) __WEAK_WITH_DEFAULT;
void HASH_IRQHandler (void) __WEAK_WITH_DEFAULT;
void PKA_IRQHandler (void) __WEAK_WITH_DEFAULT;
void CEC_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM12_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM13_IRQHandler (void) __WEAK_WITH_DEFAULT;
void TIM14_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I3C1_EV_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I3C1_ER_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C4_EV_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I2C4_ER_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPTIM3_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPTIM4_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPTIM5_IRQHandler (void) __WEAK_WITH_DEFAULT;
void LPTIM6_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I3C2_EV_IRQHandler (void) __WEAK_WITH_DEFAULT;
void I3C2_ER_IRQHandler (void) __WEAK_WITH_DEFAULT;
void COMP_IRQHandler (void) __WEAK_WITH_DEFAULT;

typedef void(*VECTOR_TABLE_Type)(void);

// Cortex-M33 supports 480 external interrupts + 16 internal lines
//  thus the vector table size is 496
// __VECTOR_TABLE and __VECTOR_TABLE_ATTRIBUTE are also defined in CMSIS
extern const VECTOR_TABLE_Type __VECTOR_TABLE[496];
       const VECTOR_TABLE_Type __VECTOR_TABLE[496] __VECTOR_TABLE_ATTRIBUTE = {
  (VECTOR_TABLE_Type)(&__INITIAL_SP),       /*     Initial Stack Pointer */
  Reset_Handler,                            /*     Reset Handler */
  NMI_Handler,                              /* -14 NMI Handler */
  HardFault_Handler,                        /* -13 Hard Fault Handler */
  MemManage_Handler,                        /* -12 MPU Fault Handler */
  BusFault_Handler,                         /* -11 Bus Fault Handler */
  UsageFault_Handler,                       /* -10 Usage Fault Handler */
  SecureFault_Handler,                      /*  -9 Secure Fault Handler */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  0,                                        /*     Reserved */
  SVC_Handler,                              /*  -5 SVCall Handler */
  DebugMon_Handler,                         /*  -4 Debug Monitor Handler */
  0,                                        /*     Reserved */
  PendSV_Handler,                           /*  -2 PendSV Handler */
  SysTick_Handler,                          /*  -1 SysTick Handler */
  // the IRQHandler list below is created from reference manuals
  // this list is a superset, not all devices support all
  // starts from position=0 to 133
  WWDG_IRQHandler,
  PVD_AVD_IRQHandler,
  RTC_IRQHandler,
  RTC_S_IRQHandler,
  TAMP_IRQHandler,
  RAMCFG_IRQHandler,
  FLASH_IRQHandler,
  FLASH_S_IRQHandler,
  GTZC_IRQHandler,
  RCC_IRQHandler,
  RCC_S_IRQHandler,
  EXTI0_IRQHandler,
  EXTI1_IRQHandler,
  EXTI2_IRQHandler,
  EXTI3_IRQHandler,
  EXTI4_IRQHandler,
  EXTI5_IRQHandler,
  EXTI6_IRQHandler,
  EXTI7_IRQHandler,
  EXTI8_IRQHandler,
  EXTI9_IRQHandler,
  EXTI10_IRQHandler,
  EXTI11_IRQHandler,
  EXTI12_IRQHandler,
  EXTI13_IRQHandler,
  EXTI14_IRQHandler,
  EXTI15_IRQHandler,
  GPDMA1_Channel0_IRQHandler,
  GPDMA1_Channel1_IRQHandler,
  GPDMA1_Channel2_IRQHandler,
  GPDMA1_Channel3_IRQHandler,
  GPDMA1_Channel4_IRQHandler,
  GPDMA1_Channel5_IRQHandler,
  GPDMA1_Channel6_IRQHandler,
  GPDMA1_Channel7_IRQHandler,
  IWDG_IRQHandler,
  SAES_IRQHandler,
  ADC1_IRQHandler,
  DAC1_IRQHandler,
  FDCAN1_IT0_IRQHandler,
  FDCAN1_IT1_IRQHandler,
  TIM1_BRK_IRQHandler,
  TIM1_UP_IRQHandler,
  TIM1_TRG_COM_IRQHandler,
  TIM1_CC_IRQHandler,
  TIM2_IRQHandler,
  TIM3_IRQHandler,
  TIM4_IRQHandler,
  TIM5_IRQHandler,
  TIM6_IRQHandler,
  TIM7_IRQHandler,
  I2C1_EV_IRQHandler,
  I2C1_ER_IRQHandler,
  I2C2_EV_IRQHandler,
  I2C2_ER_IRQHandler,
  SPI1_IRQHandler,
  SPI2_IRQHandler,
  SPI3_IRQHandler,
  USART1_IRQHandler,
  USART2_IRQHandler,
  USART3_IRQHandler,
  UART4_IRQHandler,
  UART5_IRQHandler,
  LPUART1_IRQHandler,
  LPTIM1_IRQHandler,
  TIM8_BRK_IRQHandler,
  TIM8_UP_IRQHandler,
  TIM8_TRG_COM_IRQHandler,
  TIM8_CC_IRQHandler,
  ADC2_IRQHandler,
  LPTIM2_IRQHandler,
  TIM15_IRQHandler,
  TIM16_IRQHandler,
  TIM17_IRQHandler,
  USB_DRD_FS_IRQHandler,
  CRS_IRQHandler,
  UCPD1_IRQHandler,
  FMC_IRQHandler,
  OCTOSPI1_IRQHandler,
  SDMMC1_IRQHandler,
  I2C3_EV_IRQHandler,
  I2C3_ER_IRQHandler,
  SPI4_IRQHandler,
  SPI5_IRQHandler,
  SPI6_IRQHandler,
  USART6_IRQHandler,
  USART10_IRQHandler,
  USART11_IRQHandler,
  SAI1_IRQHandler,
  SAI2_IRQHandler,
  GPDMA2_Channel0_IRQHandler,
  GPDMA2_Channel1_IRQHandler,
  GPDMA2_Channel2_IRQHandler,
  GPDMA2_Channel3_IRQHandler,
  GPDMA2_Channel4_IRQHandler,
  GPDMA2_Channel5_IRQHandler,
  GPDMA2_Channel6_IRQHandler,
  GPDMA2_Channel7_IRQHandler,
  UART7_IRQHandler,
  UART8_IRQHandler,
  UART9_IRQHandler,
  UART12_IRQHandler,
  SDMMC2_IRQHandler,
  FPU_IRQHandler,
  ICACHE_IRQHandler,
  DCACHE1_IRQHandler,
  ETH_IRQHandler,
  ETH_WKUP_IRQHandler,
  DCMI_PSSI_IRQHandler,
  FDCAN2_IT0_IRQHandler,
  FDCAN2_IT1_IRQHandler,
  CORDIC_IRQHandler,
  FMAC_IRQHandler,
  DTS_IRQHandler,
  RNG_IRQHandler,
  OTFDEC1_IRQHandler,
  AES_IRQHandler,
  HASH_IRQHandler,
  PKA_IRQHandler,
  CEC_IRQHandler,
  TIM12_IRQHandler,
  TIM13_IRQHandler,
  TIM14_IRQHandler,
  I3C1_EV_IRQHandler,
  I3C1_ER_IRQHandler,
  I2C4_EV_IRQHandler,
  I2C4_ER_IRQHandler,
  LPTIM3_IRQHandler,
  LPTIM4_IRQHandler,
  LPTIM5_IRQHandler,
  LPTIM6_IRQHandler,
  I3C2_EV_IRQHandler,
  I3C2_ER_IRQHandler,
  COMP_IRQHandler
};

__NO_RETURN void Reset_Handler(void)
{
  __set_PSP((uint32_t)(&__INITIAL_SP));

  __set_MSPLIM((uint32_t)(&__STACK_LIMIT));
  __set_PSPLIM((uint32_t)(&__STACK_LIMIT));

  // if FPU is present, enable the access
  #if (__FPU_PRESENT == 1)
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
  #endif

  // disable unaligned access support
  // unaligned accesses will generate usage fault
  // quite difficult to use, everything has to be aligned, memcpy etc.
  // SCB->CCR |= SCB_CCR_UNALIGN_TRP_Msk;

  SystemInit();
  __PROGRAM_START();
}

volatile uint32_t stack_pointer = 0;

void HardFault_Handler(void)
{
    __asm__("movs r0, #3");
    __asm__("mov r1, lr");
    __asm__("tst r1, r0");
    __asm__("ite ne");
    __asm__("mrsne r1, psp");
    __asm__("mrseq r1, msp");
    __asm__("ldr r0, =stack_pointer");
    __asm__("str r1, [r0]");

    HardFault_Callback((void*) stack_pointer);
}

void Default_Handler(void)
{
  while (1);
}
