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

/*
 * This file is based on CMSIS_5.9.0/Device/ARM/ARMCM33/Source/GCC/gcc_arm.ld
 *
 * Modifications:
 * - Following sections are removed:
 * -- .ARM.extab and .ARM.exidx output sections
 * -- .gnu.sgstubs output section
 * -- .ctors, .dtors input sections 
 * -- .eh_frame input sections
 * -- vtable input section
 * -- preinit, init and fini array input sections and symbols
 * -- jcr section
 * -- COMMON input section 
 * -- .stackseal output section
 * - ROM and RAM constants are modified according to STM32H5.
 * - .copy.table and .zero.table makes code segment RWX
 *   to overcome this, segments are defined explicitly (PHDRS)
 */

__ROM_BASE = 0x08000000;
__ROM_SIZE = 2048K;

__RAM_BASE = 0x20000000;
__RAM_SIZE = 640K;

__STACK_SIZE = 16K;
__HEAP_SIZE  = 256K;

MEMORY
{
  FLASH (rx)  : ORIGIN = __ROM_BASE, LENGTH = __ROM_SIZE
  RAM   (rwx) : ORIGIN = __RAM_BASE, LENGTH = __RAM_SIZE
}

ENTRY(Reset_Handler)

/* having .copy.table and .zero.table like below
 * makes TEXT segment RWX
 * although it seems to cause no problems, it is not ideal.
 * if the segments are manually defined below with given FLAGS
 * then the flags of final segments are correct.
 * when PHDRS is defined, data and bss is not automatically created
 * so all three segments have to be defined in PHDS
 * difference between TEXT and DATA/BSS is TEXT is RX, DATA/BSS is RW
 * difference between DATA and BSS is BSS has p_filesz=0
 * p_filesz=0 means BSS occupies no space in ROM/FILESYSTEM
 */
PHDRS 
{
  text PT_LOAD FLAGS(5);
  data PT_LOAD FLAGS(6);
  bss PT_LOAD FLAGS(6);
}

SECTIONS
{
  .text :
  {
    KEEP(*(.vectors))
    *(.text*)
    /* .init and .fini are used by C runtime, they have to exist */
    KEEP(*(.init))
    KEEP(*(.fini))
    *(.rodata*) 
  } >FLASH :text

  .copy.table : {
    /* this structure is used by _cmsis_start to initialize .data */
    . = ALIGN(4);
    __copy_table_start__ = .;
    LONG (__etext)
    LONG (__data_start__)
    LONG ((__data_end__ - __data_start__) / 4)
    __copy_table_end__ = .;
  } > FLASH :text

  .zero.table : {
    . = ALIGN(4);
    /* libgloss crt0.S already zeroes bss, so there is no need for this actually */ 
    /* that is why it contains nothing and garbage collected by the linker */
    __zero_table_start__ = .;
    __zero_table_end__ = .;
  } > FLASH :text

  __etext = ALIGN (4);

  .data : AT (__etext)
  {
    __data_start__ = .;
    *(.data)
    *(.data.*)
    . = ALIGN(4);
    __data_end__ = .;
  } > RAM :data

  .bss :
  {
    . = ALIGN(4);
    __bss_start__ = .;
    *(.bss)
    *(.bss.*)
    /* *(COMMON) means all uninitialized data that is not allocated until now */
    *(COMMON)
    . = ALIGN(4);
    __bss_end__ = .;
  } > RAM AT > RAM :bss

  .heap (COPY) :
  {
    . = ALIGN(8);
    __end__ = .;
    /* this is used by default libnosys _sbrk implementation */
    PROVIDE(end = .);
    . = . + __HEAP_SIZE;
    . = ALIGN(8);
    __HeapLimit = .;
  } > RAM

  .stack (ORIGIN(RAM) + LENGTH(RAM) - __STACK_SIZE) (COPY) :
  {
    . = ALIGN(8);
    __StackLimit = .;
    . = . + __STACK_SIZE;
    . = ALIGN(8);
    __StackTop = .;
  } > RAM
  /* __stack is used by C runtime, it has to exist */
  PROVIDE(__stack = __StackTop);

  /* remove .ARM.exidx .ARM.extab etc. */
  /DISCARD/ : 
  {
    *(.ARM.*)
  }
  
  /* Check if data + heap + stack exceeds RAM limit */
  ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
