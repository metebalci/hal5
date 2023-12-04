
# HAL5

*This is still a work-in-progress.*

HAL5 is a high-level system API for STM32H5 Cortex-M33 MCUs, particularly for STM32H563/573. It does not depend on STM32 HAL, but only CMSIS. It is not a HAL per se since the current aim is to support only STM32H5 series, however the functionality is like HAL and even higher level than a HAL.

The main purpose at the moment is neither to support all features of the MCU nor to have production quality, but to provide an easy and a robust API to learn, study and experiment with STM32H5 MCUs. HAL5 is a result of my frustration with existing APIs and the way I prefer to study the MCUs.

The only two dependencies of the project are [CMSIS (Arm) 5.9.0](https://github.com/ARM-software/CMSIS_5) and [STM32CubeH5 CMSIS Device MCU Component (ST) 1.1.0](https://github.com/STMicroelectronics/cmsis_device_h5). STM32CubeH5 is used for TypeDefs, Register, Msk and Pos macro definitions and macro functions like CLEAR_BIT, SET_BIT, MODIFY_REG.

The source code in this repository can be divided into four groups:

- hal5 source files: provides hal5 system API and implementation. All files start with `hal5`. These are built to a static library named `hal5.a`. Only `hal5.h` needs to be included by the users and the program should be linked with `hal5.a`.

- startup file and link script: `startup_stm32h563.c` and `startup.ld`.

- board support package: `bsp.h` and `bsp_nucleo_h563zi.c` as an example to support NUCLEO-H563ZI with the test project.

- test project: includes `main.c` and `syscalls.c`.

Startup, board support and test project files are used to build the test project `hal5.elf`.

# Main Features

- API limits the ambiguity and error-prone operations i.e. GPIO configuration can only be made for valid configurations. This is mostly accomplished by using enums for the input and output arguments of public functions, no bit fields or masks are used.

- No error is returned from most of the API functions. This is because most, if not all, of these functions are related to hardware and there is no temporary errors. A call resulting an error will always result an error with the same parameters, so it should not be handled in the software. Instead, assertions are often used to see where the issue is. For example, if something is supplied for PLL configuration that is impossible to satisfy in the hardware, no error is returned but an assertion is failed within the relevant function.

- API provides functions returning the actual frequency of various clocks in the clock tree e.g. `hal5_rcc_pll1_p_ck()`. These values are not cached (and there is no clock update function) but queried from the hardware when requested.

- CMSIS SysTick_Config is not used but a System tick (actually two ticks) is implemented, one in millisecond, the other is in second resolution.

- The startup code is C-based, not assembly.

## Console

- A simple VT100-like console (on LPUART1) is provided for information and debug purposes. `printf` writes to this console (LPUART1), and assert works with it as well. It can be used with 921600 baud and the default configuration is 8N1. For the terminal emulation on PC, `minicom` can be used with `addcarreturn` option.

## Fault Reporting

`hal5_dump_cfsr_info` function prints information about the fault to the console. This can be called in HardFault handler.

The purpose of `bsp_fault` function is to visually indicate a fault (possibly HardFault) happened on the board. I use the red LED on the board for this purpose.

The `HardFault_Handler` in `main.c` uses both functions. 

## Ease of use 

The system core clock (`sys_ck`) can be changed with `hal5_change_sys_ck` which automatically adjust flash latency and voltage scaling.

The system core clock (`sys_ck`) can be changed to `pll1_p` with `hal5_change_sys_ck_to_pll1_p` with a single target frequency parameter. This will search for a PLL configuration, initialize it, and change `sys_ck` to `pll1_p` (and automatically adjusts flash latency and voltage scaling).

# Test Project

```
Console is LPUART1. 921600, 8N1.
Booting...
Due to CPU reset...
ICACHE enabled.
Prefetch enabled.
PLL config is found: /M=2 xN=30 /P=2.
PLL1 initialized.
SYSCLK is now PLL1_P.
MCO2 shows SYSCLK.
SYSTICK configured.
RNG enabled. [9B02FC8D]
Boot completed.
```

# Startup and Linker Script

Based on CMSIS C-based startup (`startup_ARMCM33.c`) and linker script (`gnu_arm.ld`), a C-based startup is used.

Reset_Handler calls `__PROGRAM_START`, this is defined as `__cmsis_start`. This is implemented in `cmsis_gcc.h`, and it uses copy_table and zero_table for .data and .bss initialization.

`SystemInit` function call is kept but it is made a weak symbol, so it is not a must. Most initialization is expected to be done in C `main`. Because the clock speed is not increased, .data and .bss initialization are slower. `SystemInit` can still be used if absolutely necessary. FPU access is always enabled if FPU is present (`__FPU_PRESENT`).

# Peripherals Support

Core functionality of small number of peripherals are supported.

- LPUART supports LPUART1 for console. 
- I2C supports I2C2, because it is convenient to use I2C2 pins on NUCLEO-H563ZI board.

Peripheral routines are not runtime configurable in the sense that I2C support cannot be changed to I2C1 without re-compiling the library.

USB support is in another repo: [hal5_usb](https://github.com/metebalci/hal5_usb).

# Build and Test

[ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) is required to build the source code. Other compilers are not tested and probably will require small changes.

[STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) is required to flash the firmware.

The project is tested with a [NUCLEO-H563ZI development board](https://www.st.com/en/evaluation-tools/nucleo-h563zi.html).

`make` builds the library (`hal5.a`) and the firmware (`hal5.elf`) containing the test project. Before building, it downloads (clones) CMSIS 5.9.0 and STM32CubeH5 1.1.0 from github.

`make flash` builds the firmware containing the test project and programs the firmware to the MCU using STM32_Programmer_CLI.

# References

- [Reference Manual of STM32H563/H573](https://www.st.com/resource/en/reference_manual/rm0481-stm32h563h573-and-stm32h562-armbased-32bit-mcus-stmicroelectronics.pdf)

- [Datasheet of STM32H563](https://www.st.com/resource/en/datasheet/stm32h562ag.pdf)

- [CMSIS Version 5.9.0](https://www.keil.com/pack/doc/CMSIS/General/html/index.html)

# License

SPDX-FileCopyrightText: 2023 Mete Balci

SPDX-License-Identifier: Apache-2.0

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
