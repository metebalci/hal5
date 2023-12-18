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
#include <stdlib.h>
#include <string.h>

#include <stm32h5xx.h>

#include "hal5.h"
#include "hal5_private.h"

static uint8_t digest[64];

uint32_t hal5_hash_get_digest_size(hal5_hash_algorithm_t algorithm)
{
    switch (algorithm)
    {
        case hal5_hash_sha1:
            return 20; 
        case hal5_hash_sha2_224:
        case hal5_hash_sha2_512_224:
            return 28;

        case hal5_hash_sha2_256:
        case hal5_hash_sha2_512_256:
            return 32;

        case hal5_hash_sha2_384:
            return 48;

        case hal5_hash_sha2_512:
            return 64;

        default:
            assert (false);
    }
}

static uint32_t get_algorithm_encoding(hal5_hash_algorithm_t algorithm)
{
    switch (algorithm)
    {
        case hal5_hash_sha1:
            return 0b0000;

        case hal5_hash_sha2_224:
            return 0b0010;

        case hal5_hash_sha2_256:
            return 0b0011;

        case hal5_hash_sha2_384:
            return 0b1100;

        case hal5_hash_sha2_512_224:
            return 0b1101;

        case hal5_hash_sha2_512_256:
            return 0b1110;

        case hal5_hash_sha2_512:
            return 0b1111;

        default:
            assert (false);
    }

    return 0;
}

static hal5_hash_algorithm_t get_algorithm(void)
{
    const uint32_t algorithm = (HASH->CR & HASH_CR_ALGO_Msk) >> HASH_CR_ALGO_Pos;

    switch (algorithm)
    {
        case 0b0000:
            return hal5_hash_sha1;

        case 0b0010:
            return hal5_hash_sha2_224;

        case 0b0011:
            return hal5_hash_sha2_256;

        case 0b1100:
            return hal5_hash_sha2_384;

        case 0b1101:
            return hal5_hash_sha2_512_224;

        case 0b1110:
            return hal5_hash_sha2_512_256;

        case 0b1111:
            return hal5_hash_sha2_512;

        default:
            assert (false);
    }

    return 0;
}

void hal5_hash_enable(void) 
{
    hal5_rcc_enable_hash();
}

static uint32_t block_size;
static uint32_t max_word_index;
static uint32_t nblw;
static uint32_t word_index;

void hal5_hash_init_for_hash(
        const hal5_hash_algorithm_t algorithm)
{
    const uint32_t encoded_algorithm = get_algorithm_encoding(algorithm);

    // select algorithm
    MODIFY_REG(
            HASH->CR,
            HASH_CR_ALGO_Msk,
            encoded_algorithm << HASH_CR_ALGO_Pos);

    // initialize
    MODIFY_REG(
            HASH->CR,
            HASH_CR_INIT_Msk,
            1 << HASH_CR_INIT_Pos);

    block_size = (((HASH->SR & HASH_SR_NBWE_Msk) >> HASH_SR_NBWE_Pos) - 1) * 4;
    max_word_index = block_size >> 2;

    // select hash mode
    MODIFY_REG(
            HASH->CR,
            HASH_CR_MODE_Msk,
            0 << HASH_CR_MODE_Pos);

    // select data swapping, use DIN as 4x 8-bit data or bytes
    MODIFY_REG(
            HASH->CR,
            HASH_CR_DATATYPE_Msk,
            0b10 << HASH_CR_DATATYPE_Pos);

    // initialize
    MODIFY_REG(
            HASH->CR,
            HASH_CR_INIT_Msk,
            1 << HASH_CR_INIT_Pos);

    nblw = 0;

    word_index = 0;
}

void hal5_hash_update(
        const uint8_t* data, 
        const uint32_t offset,
        const uint32_t len)
{
    // word_size: actual bytes in the word
    uint32_t word_size = len - offset;
    if (word_size > 4) word_size = 4;
    if (word_size == 0) return;

    // if this is the first word if the block
    // wait for previous block processing to finish
    if (word_index == 0)
    {
        while ((HASH->SR & HASH_SR_DINIS_Msk) == 0);
    }

    uint32_t din = 0UL;

    if (word_size == 4UL)
    {
        din = ((data[offset+3] << 24) |
                (data[offset+2] << 16) |
                (data[offset+1] << 8) |
                (data[offset]));

        nblw = 0UL;
    }
    else if (word_size == 3UL)
    {
        din = ((data[offset+2] << 16) |
                (data[offset+1] << 8) |
                (data[offset]));

        nblw = 24UL;

    }
    else if (word_size == 2UL)
    {
        din = (data[offset+1] << 8) | data[offset];

        nblw = 16UL;
    }
    else if (word_size == 1UL)
    {
        din = data[offset];

        nblw = 8UL;
    }

    HASH->DIN = din;

    word_index++;
    if (word_index == max_word_index) word_index = 0;
}

void hal5_hash_finalize(void)
{
    // configure last word padding
    MODIFY_REG(
            HASH->STR,
            HASH_STR_NBLW_Msk,
            nblw << HASH_STR_NBLW_Pos);

    // start digest calculation
    MODIFY_REG(
            HASH->STR,
            HASH_STR_DCAL_Msk,
            1 << HASH_STR_DCAL_Pos);
    
    // wait for digest calculation completion
    while ((HASH->SR & HASH_SR_DCIS_Msk) == 0);

    // copy all digest registers
    // caller will know the digest size
    for (uint32_t i = 0; i < 16; i++)
    {
        const uint32_t d = HASH_DIGEST->HR[i];

        digest[i*4]     = (d >> 24) & 0xFF; 
        digest[i*4+1]   = (d >> 16) & 0xFF; 
        digest[i*4+2]   = (d >> 8) & 0xFF; 
        digest[i*4+3]   = d & 0xFF; 
    }
}

uint8_t* hal5_hash_get_digest()
{
    return digest;
}

#if defined(HAL5_CAVP_SHA1_TESTS) || \
    defined(HAL5_CAVP_SHA256_TESTS) || \
    defined(HAL5_CAVP_SHA512_TESTS)

#if defined(HAL5_CAVP_SHA1_TESTS)

#include "cavp-test-vectors/SHA1ShortMsg.rsp.h"
#include "cavp-test-vectors/SHA1LongMsg.rsp.h"

#endif

#if defined(HAL5_CAVP_SHA256_TESTS)

#include "cavp-test-vectors/SHA256ShortMsg.rsp.h"
#include "cavp-test-vectors/SHA256LongMsg.rsp.h"

#endif

#if defined(HAL5_CAVP_SHA512_TESTS)

#include "cavp-test-vectors/SHA512ShortMsg.rsp.h"
#include "cavp-test-vectors/SHA512LongMsg.rsp.h"

#endif


static void str2bytes(const char* in, uint8_t* out, uint32_t outlen)
{
    if (*in == '\0') return;

    uint32_t offset = 0;

    while (true)
    {
        assert (offset < outlen);
        out[offset] = *in;
        offset++;
        in++;
        if (*in == '\0') return;
    }
}

static uint8_t hexchar2nibble(const char c)
{
    if (c < 48) assert (false);
    else if (c <= 57)
    {
        // 0-9
        return (c - 48);
    }
    else if (c < 65) assert (false);
    else if (c <= 70) 
    {
        // A-F
        return (c - 65) + 10;
    }
    else if (c < 97) assert (false);
    else if (c <= 102)
    {
        // a-f
        return (c - 97) + 10;
    }
    else assert (false);
}

static uint32_t hexstr2bytes(const char* in, uint8_t* out, uint32_t len)
{
    if (*in == '\0') return 0;

    uint32_t offset = 0;

    while (true)
    {
        assert (offset < len);

        out[offset] = hexchar2nibble(*in);
        in++;
        if (*in == '\0') return offset+1;

        out[offset] = (out[offset] << 4) | hexchar2nibble(*in);
        in++;
        if (*in == '\0') return offset+1;

        offset++;
    }

    return offset;
}

static bool cmpbytes(uint8_t* i1, uint8_t* i2, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        if (i1[i] != i2[i]) return false;
    }
    return true;
}

static void print_bytes(
        const char* label,
        uint8_t* buf, 
        uint32_t len)
{
    printf("%s", label);
    for (uint32_t i = 0; i < len; i++)
    {
        printf("%02X", buf[i]);
    }
    printf("\n");
}

void cavp_hash_test(
        hal5_hash_algorithm_t algorithm, 
        const char** rsp)
{
    while (*rsp != NULL)
    {
        const char* input = *rsp;
        uint32_t input_len = strlen(input)/2;
        rsp++;

        const char* expected = *rsp;
        rsp++;

        uint8_t* input_buf = (uint8_t*) malloc(input_len);
        hexstr2bytes(input, input_buf, input_len);

        uint8_t correct_digest[64];
        hexstr2bytes(expected, correct_digest, 64);

        hal5_hash_init_for_hash(algorithm);

        for (uint32_t i = 0; i < input_len; i += 4)
        {
            hal5_hash_update(input_buf, i, input_len);
        }

        hal5_hash_finalize();

        uint8_t* calculated_digest = hal5_hash_get_digest();

        if (cmpbytes(
                    calculated_digest, 
                    correct_digest, 
                    hal5_hash_get_digest_size(algorithm)))
        {
            printf(".");
            fflush(stdout);
        }
        else 
        {
            printf("Len:%lu test failed\n", input_len);
            print_bytes("cor_digest: ", correct_digest, 20);
            print_bytes("cal_digest: ", calculated_digest, 20);

            assert (false);
        }

        free(input_buf);
    }
    printf("\n");
}

#endif
       
void hal5_hash_test()
{
#ifdef HAL5_CAVP_SHA1_TESTS
    printf("SHA1 short tests:\n");

    cavp_hash_test(
            hal5_hash_sha1, 
            cavp_test_vectors_sha1shortmsg_rsp);

    printf("SHA1 long tests:\n");

    cavp_hash_test(
            hal5_hash_sha1, 
            cavp_test_vectors_sha1longmsg_rsp);
#endif
#ifdef HAL5_CAVP_SHA256_TESTS
    printf("SHA256 short tests:\n");

    cavp_hash_test(
            hal5_hash_sha2_256,
            cavp_test_vectors_sha256shortmsg_rsp);

    printf("SHA256 long tests:\n");

    cavp_hash_test(
            hal5_hash_sha2_256, 
            cavp_test_vectors_sha256longmsg_rsp);
#endif
#ifdef HAL5_CAVP_SHA512_TESTS
    printf("SHA512 short tests:\n");

    cavp_hash_test(
            hal5_hash_sha2_512,
            cavp_test_vectors_sha512shortmsg_rsp);

    printf("SHA512 long tests:\n");

    cavp_hash_test(
            hal5_hash_sha2_512, 
            cavp_test_vectors_sha512longmsg_rsp);
#endif

}
