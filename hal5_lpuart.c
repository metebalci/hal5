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

#include <assert.h>
#include <stdio.h>

#include <stm32h5xx.h>

#include "hal5.h"

static uint32_t lpuart_baud;

void hal5_lpuart_configure(
        const uint32_t baud)
{
    assert (baud <= 921600);
    assert (baud >= 115200);

    lpuart_baud = baud;

    hal5_rcc_enable_csi();

    // using LPUART1 as console
    // PB6 is TX, PB7 is RX, both AF8
    hal5_gpio_configure_as_af(
            PB6,
            af_pp_floating,
            high_speed,
            AF8);

    hal5_gpio_configure_as_af(
            PB7,
            af_pp_pull_up,
            high_speed,
            AF8);

    // enable LPUART1 clock
    hal5_rcc_enable_lpuart1();

    // set lpuart_ker_ck to csi_ker_ck
    hal5_rcc_change_lpuart1_ker_ck(lpuart1sel_csi_ker_ck);

    // PRESC is not needed for 115200 to 921600 bauds with csi (or hsi)
    /*
       MODIFY_REG(LPUART1->PRESC, LPUART_PRESC_PRESCALER_Msk,
       0b0000 << LPUART_PRESC_PRESCALER_Pos);
       */

    // original equation is 256 * clock / baud
    // to not reach 32-bit with multiplication, first division is performed
    // because a standard baud is already an integer factor of 256
    assert (lpuart_baud % 256 == 0);
    const uint32_t BRR = hal5_rcc_get_lpuart1_ker_ck() / (lpuart_baud / 256);
    assert (BRR >= 0x300);
    assert (BRR <= 0xFFFFF);
    LPUART1->BRR = BRR;

    // enable FIFO
    SET_BIT(LPUART1->CR1, USART_CR1_FIFOEN);
    // enable UART
    SET_BIT(LPUART1->CR1, USART_CR1_UE);
    // enable transmit
    SET_BIT(LPUART1->CR1, USART_CR1_TE);
    // enable receive
    SET_BIT(LPUART1->CR1, USART_CR1_RE);
}

void hal5_lpuart_write(
        const char ch)
{
    // TXE and TXFNF bit numbers are same
    // TXE is when FIFO is disabled, TXFNF otherwise
    while ((LPUART1->ISR & USART_ISR_TXE_Msk) == 0);
    LPUART1->TDR = ch;
}

bool hal5_lpuart_read(
        char* ch)
{
    if (LPUART1->ISR & USART_ISR_RXNE_Msk) {
        *ch = LPUART1->RDR;
        return true;
    } else {
        return false;
    }
}

void hal5_lpuart_dump_info()
{
    printf("LPUART1 on PB6 TX, PB7 RX.\n");
    printf("%lu baud, 8N1.\n", lpuart_baud);
}
