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
#include "hal5_private.h"

// this table is from the reference manual
// only lists frequencies
// rows are as in rows array below
// columns are as in cols array below
// cols are ordered as below
// wrhighfreq only depends on latency, so not listed here
static const uint32_t latency_table[6][4] = 
{
    { 20,  30,  34,  42},
    { 40,  60,  68,  84},
    { 60,  90, 102, 126},
    { 80, 120, 136, 168},
    {100, 150, 170, 210},
    {  0,   0, 200, 250}
};

static const hal5_pwr_voltage_scaling_t cols[] = {
    hal5_pwr_vos3, hal5_pwr_vos2, hal5_pwr_vos1, hal5_pwr_vos0};

static const hal5_flash_latency_t rows[] = 
{
    hal5_flash_0ws,
    hal5_flash_1ws,
    hal5_flash_2ws,
    hal5_flash_3ws,
    hal5_flash_4ws,
    hal5_flash_5ws,
};

static const uint32_t num_rows = sizeof(rows);
static const uint32_t num_cols = sizeof(cols);

// optimizing power means the minimum voltage (highest voltage scaling)
static bool find_optimizing_power(
        const uint32_t freq,
        uint32_t* prow,
        uint32_t* pcol)
{
    // search by cols (so find the minimum col)
    for (uint32_t col = 0; col < num_cols; col++)
    {
        for (uint32_t row = 0; row < num_rows; row++)
        {
            if (freq < latency_table[row][col]) 
            {
                *prow = row;
                *pcol = col;
                return true;
            }
        }
    }
    return false;
}

// optimizing performance mean the minimum latency
static bool find_optimizing_performance(
        uint32_t freq,
        uint32_t* prow,
        uint32_t* pcol)
{
    // search by rows (so find the minimum latency)
    for (uint32_t row = 0; row < num_rows; row++)
    {
        for (uint32_t col = 0; col < num_cols; col++)
        {
            if (freq < latency_table[row][col]) 
            {
                *prow = row;
                *pcol = col;
                return true;
            }
        }
    }
    return false;
}

bool hal5_flash_calculate_latency(
        const uint32_t freq,
        const bool optimize_power,
        hal5_flash_latency_t* latency,
        hal5_pwr_voltage_scaling_t* vos)
{
    uint32_t row;
    uint32_t col; 

    if (optimize_power)
    {
        if (!find_optimizing_power(freq, &row, &col)) return false;
    }
    else
    {
        if (!find_optimizing_performance(freq, &row, &col)) return false;
    }

    *latency = rows[row];
    *vos = cols[col];

    return true;
}

void hal5_flash_change_latency(
        hal5_flash_latency_t latency)
{
    uint32_t wrhighfreq_bits;
    uint32_t latency_bits;
    switch (latency)
    {
        case hal5_flash_0ws: wrhighfreq_bits = 0b00; latency_bits = 0b0000; break;
        case hal5_flash_1ws: wrhighfreq_bits = 0b00; latency_bits = 0b0001; break;
        case hal5_flash_2ws: wrhighfreq_bits = 0b01; latency_bits = 0b0010; break;
        case hal5_flash_3ws: wrhighfreq_bits = 0b01; latency_bits = 0b0011; break;
        case hal5_flash_4ws: wrhighfreq_bits = 0b10; latency_bits = 0b0100; break;
        case hal5_flash_5ws: wrhighfreq_bits = 0b10; latency_bits = 0b0101; break;
        default: assert (false);
    }
    MODIFY_REG(FLASH->ACR, FLASH_ACR_WRHIGHFREQ_Msk,
            wrhighfreq_bits << FLASH_ACR_WRHIGHFREQ_Pos);

    // wait until it is applied
    while (((FLASH->ACR & FLASH_ACR_WRHIGHFREQ_Msk) 
                >> FLASH_ACR_WRHIGHFREQ_Pos) != wrhighfreq_bits);

    MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY_Msk,
            latency_bits << FLASH_ACR_LATENCY_Pos);

    // wait until it is applied
    while (((FLASH->ACR & FLASH_ACR_LATENCY_Msk) 
                >> FLASH_ACR_LATENCY_Pos) != latency_bits);
}

void hal5_flash_enable_prefetch()
{
    SET_BIT(FLASH->ACR, FLASH_ACR_PRFTEN);

    while ((FLASH->ACR & FLASH_ACR_PRFTEN) == 0);
}
