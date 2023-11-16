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

#include <stm32h5xx.h>

#include "hal5.h"

#ifdef NDEBUG

// newlib uses __assert_func
// glibc uses __assert_fail

// this should not return
// return address is not saved by the caller
    _ATTRIBUTE ((__noreturn__))
void __assert_func(const char *file,
        int line,
        const char *func,
        const char *failedexpr) 
{
    printf("ASSERT %s:%d %s() %s\n", file, line, func, failedexpr);

    hal5_freeze();
}

#endif
