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

#ifndef __HAL5_PRIVATE_H__
#define __HAL5_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define CONSOLE(f_, ...) printf((f_), ##__VA_ARGS__)

// these are used for printf-style debugging during development
// in case of a name clash, they can be removed/commented out
#define SHOWP(p) (CONSOLE("" #p ": %p\n", (void*)p))
#define SHOWU(u) (CONSOLE("" #u ": %lu (0x%08lX)\n", (uint32_t)u, (uint32_t)u))
#define MARK(n) (CONSOLE("----- MARK %d -----\n", n))
#define MARKA (CONSOLE("----- MARK A -----\n"))
#define MARKB (CONSOLE("----- MARK B -----\n"))

// https://stackoverflow.com/a/58532788/313811
#define HAL5_MAX(a,b)        \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a > _b ? _a : _b;       \
})

// https://stackoverflow.com/a/58532788/313811
#define HAL5_MIN(a,b)        \
({                           \
    __typeof__ (a) _a = (a); \
    __typeof__ (b) _b = (b); \
    _a < _b ? _a : _b;       \
})

#ifdef __cplusplus
}
#endif

#endif
