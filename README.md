
# HAL5

*This is still a work-in-progress.*

HAL5 is a high-level system API for STM32H5 Cortex-M33 MCUs, particularly for STM32H563/573. It does not depend on STM32 HAL, but only CMSIS. It is not a HAL per se since the current aim is to support only STM32H5 series, however the functionality is like HAL and even higher level than a HAL.

The main purpose at the moment is neither to support all features of the MCU nor to have production quality, but to provide an easy and a robust API to learn, study and experiment with STM32H5 MCUs. HAL5 is a result of my frustration with existing APIs and the way I prefer to study the MCUs.

The only two dependencies of the project are [CMSIS (Arm) 5.9.0](https://github.com/ARM-software/CMSIS_5) and [STM32CubeH5 CMSIS Device MCU Component (ST) 1.1.0](https://github.com/STMicroelectronics/cmsis_device_h5). STM32CubeH5 is used for TypeDefs, Register, Msk and Pos macro definitions and macro functions like CLEAR_BIT, SET_BIT, MODIFY_REG.

The source code in this repository can be divided into four groups:

- hal5 source files: provides hal5 system API and implementation, all files starting with `hal5_`. These are built into a static library named `hal5.a`. There are multiple header files but only `hal5.h` needs to be included by the users. So, if you want to use it, you need `hal5.h` and `hal5.a`.

- startup file and link script: `startup_stm32h563.c` and `startup.ld`.

- example project: includes `main.c`, `syscalls.c` and `example_usb_device.c`.

- board support package: `bsp.h` and `bsp_nucleo_h563zi.c` as an example to support NUCLEO-H563ZI with the example project.

- helpers to create and use USB descriptors: `create_descriptors.py` is a simple utility to convert descriptors defined in `descriptors.py` to a C source file.

# Main Features

- API limits the ambiguity and error-prone operations i.e. GPIO configuration can only be made for valid configurations. This is mostly accomplished by using enums for the input and output arguments of public functions, no bit fields or masks are used.

- No error is returned from most of the API functions. This is because most, if not all, of these functions are related to hardware and there is no temporary errors. A call resulting an error will always result an error with the same parameters, so it should not be handled in the software. Instead, assertions are often used to see where the issue is. For example, if something is supplied for PLL configuration that is impossible to satisfy in the hardware, no error is returned but an assertion is failed within the relevant function.

- API provides functions returning the actual frequency of various clocks in the clock tree e.g. `hal5_rcc_pll1_p_ck()`. These values are not cached (and there is no clock update function) but queried from the hardware when requested.

- CMSIS SysTick_Config is not used but a System tick (actually two ticks) is implemented, one in millisecond, the other is in second resolution.

- The startup code is C-based, not assembly.

- Provides a clear USB device implementation

## Console

- A simple VT100-like console (on LPUART1) is provided for information and debug purposes. `printf` writes to this console (LPUART1), and assert works with it as well. It can be used with 921600 baud and the default configuration is 8N1. For the terminal emulation on PC, `minicom` can be used with `addcarreturn` option.

## Fault Reporting

`hal5_dump_cfsr_info` function prints information about the fault to the console. This can be called in HardFault handler.

The purpose of `bsp_fault` function is to visually indicate a fault (possibly HardFault) happened on the board. I use the red LED on the board for this purpose.

The `HardFault_Handler` in `main.c` uses both functions. 

## Ease of use 

The system core clock (`sys_ck`) can be changed with `hal5_change_sys_ck` which automatically adjust flash latency and voltage scaling.

The system core clock (`sys_ck`) can be changed to `pll1_p` with `hal5_change_sys_ck_to_pll1_p` with a single target frequency parameter. This will search for a PLL configuration, initialize it, and change `sys_ck` to `pll1_p` (and automatically adjusts flash latency and voltage scaling).

# Startup and Linker Script

Based on CMSIS C-based startup (`startup_ARMCM33.c`) and linker script (`gnu_arm.ld`), a C-based startup is used.

Reset_Handler calls `__PROGRAM_START`, this is defined as `__cmsis_start`. This is implemented in `cmsis_gcc.h`, and it uses copy_table and zero_table for .data and .bss initialization.

`SystemInit` function call is kept but it is made a weak symbol, so it is not a must. Most initialization is expected to be done in C `main`. Because the clock speed is not increased, .data and .bss initialization are slower. `SystemInit` can still be used if absolutely necessary. FPU access is always enabled if FPU is present (`__FPU_PRESENT`).

# Peripherals Support

Core functionality of small number of peripherals are supported.

- LPUART supports LPUART1 for console. 
- I2C supports I2C2, because it is convenient to use I2C2 pins on NUCLEO-H563ZI board.

Peripheral routines are not runtime configurable in the sense that I2C support cannot be changed to I2C1 without re-compiling the library.

# USB Support

USB Host mode is not supported.

USB Device mode is supported as follows.

## Descriptors

Descriptors are defined in python, in `descriptors.py` file. This is a human-friendly form, because:

- fields like `bLength` is calculated automatically
- bit fields like `self-powered` attribute is specified as a boolean value
- additional attributes like `append-version` can be given

Most of the fields have similar names to USB descriptor fields. For an example and documentation, check `descriptors.py` in the repository. Optional USB descriptors that depend on other fields (such as endpoint transfer type) are checked and expected to be explicitly provided or not provided, no defaults assumed to prevent errors.

String descriptors are automatically created if given in `descriptors.py`, they are not explicitly created. Only supported language is US English (0x0409), this is defined implicitly in HAL5. However, Unicode can be used in provided strings, they are encoded properly as UTF-16 in generated string descriptors.

During a build, `descriptor.py` is used by `create_descriptor.py` to generate a C source file (`hal5_usb_device_descriptors.c`) which is compiled together with the application. The descriptors are created and initialized in this C source file.

### Append Version to Product String

All fields in `descriptor.py` are used as it is except the product string value if it is not `None` and `append_version` is `True`. In this case, the return values of `hal5_usb_device_version_major_ex() and _minor_ex()` are used to create a product name like `<product>_vXX.YY`. XX and YY can be between 0 and 99, and they are shown as single digit (not 0 left-padded) if they are less than 10. I have seen this in a few devices that makes it possible to observe the firmware version without any extra tool since Device Manager in Windows, System Report in macOS, or `lsusb` in Linux shows the product string.

## Endpoint 0 / Default Control Pipe

Endpoint 0, thus default control pipe, is abstracted and implemented by `hal5_usb_device_ep0.c`. USB 2.0 FS device enumeration completes successfully.

#### Default Control Pipe Status

All (standard) USB device requests (USB 2.0 9.4) are implemented.

No-data device requests:

- `Clear Feature`
- `Set Feature`
- `Set Address`
- `Set Configuration`
- `Set Interface`

are implemented with complete parameter and state checks according to USB 2.0 spec.

All features in USB 2.0 (`Endpoint Halt`, `Device Remote Wakeup`, `Test Mode`) is passed to the USB device implementation. Test mode is not implemented.

`Set Address` and `Set Configuration` correctly handles state changes to/from address and from/to configured depending on the current state, the value of device address and configuration value.

`Set Interface` is passed to the USB device implementation.

Control write request:

- `Set Descriptor` 

is optional and it is implemented as always returning a Request Error (STALL). Thus, it cannot be used by a USB Device.

Control read requests:

- `Get Status`
- `Get Descriptor`
- `Get Configuration`
- `Get Interface`
- `Synch Frame`

are implemented with complete parameter and state checks according to USB 2.0 spec.

## USB Compliance

The code with the example USB device implementation in the repository passes USB3CV Chapter 9 Tests - USB 2. The test is performed on Windows 11 with a Renesas UPD720201 XHCI controller ([Delock 89363](https://www.delock.com/produkt/89363/merkmale.html?setLanguage=en)).

## USB Transaction vs. Pipe

Since a SETUP transaction is always 3 packets (SETUP, DATA0, ACK) and DATA0 payload is always 8 bytes, SETUP transaction is always processed as it is by the default control endpoint / endpoint 0. Since this works outside of USB device implementation, it is not very important for the user.

OUT and IN transactions can be two (DATA0/1, ACK) or multiples of these two packages, until a DATA0/1 payload with less than max packet size arrives. In effect, the data that is sent or received is the combination of all the payloads. This combination is done automatically, so IN and OUT transactions are not sent to the device implementation as it is but as `_in_stage_completed` and `_out_stage_completed` callbacks. The device implementation can read the combined data from the endpoint structure easily.

This implementation might not be ideal for some cases.

## USB Device

A device implementation should only provide descriptors and implement a few functions given in `hal5_usb_device.h` with `_ex` suffix.

An example USB Device is given in `example_usb_device.c`.

# Build and Test

[ARM GNU Toolchain](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads) is required to build the source code. Other compilers are not tested and probably will require small changes.

[STM32CubeProgrammer](https://www.st.com/en/development-tools/stm32cubeprog.html) is required to flash the firmware.

The project is tested with a [NUCLEO-H563ZI development board](https://www.st.com/en/evaluation-tools/nucleo-h563zi.html).

`make` builds the library (`hal5.a`) and the firmware (`example.elf`) containing the example project. Before building, it downloads (clones) CMSIS 5.9.0 and STM32CubeH5 1.1.0 from github.

`make flash` builds the firmware containing the example project and programs the firmware to the MCU using STM32_Programmer_CLI.

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
