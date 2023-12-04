#
# SPDX-FileCopyrightText: 2023 Mete Balci
#
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2023 Mete Balci
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# set fpu to soft, softfp or hard
# soft:   software fpu, soft abi
# softfp: hardware fpu, soft abi
# hard:   harwdare fpu, hard abi
fpu ?= soft

# FLOATFLAGS based on fpu above
ifeq ($(fpu), softfp)
	FLOATFLAGS := -mfloat-abi=softfp -mfpu=fpv5-sp-d16
else ifeq ($(fpu), hard)
	FLOATFLAGS := -mfloat-abi=hard -mfpu=fpv5-sp-d16
else
	FLOATFLAGS := -mfloat-abi=soft
endif

CFLAGS := -std=gnu11
CFLAGS += -mcpu=cortex-m33 -mthumb
CFLAGS += -O0 -g
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += $(FLOATFLAGS)
CFLAGS += -I. -Icmsis/CMSIS/Core/Include -Icmsis_device_h5/Include -DSTM32H563xx
CFLAGS += --specs=nano.specs
CFLAGS += -Wall -Werror
CFLAGS += -Wno-unused-variable -Wno-unused-function 
CFLAGS += -fmax-errors=5

LDFLAGS := -mcpu=cortex-m33 -mthumb 
LDFLAGS += $(FLOATFLAGS)
LDFLAGS += --specs=nosys.specs 
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -static
LDFLAGS += --specs=nano.specs
LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group

HAL5_OBJS := hal5.o hal5_assert.o 
HAL5_OBJS += hal5_cache.o hal5_console.o hal5_crs.o
HAL5_OBJS += hal5_flash.o hal5_gpio.o hal5_i2c.o hal5_lpuart.o
HAL5_OBJS += hal5_pwr.o hal5_rcc.o hal5_rcc_ck.o
HAL5_OBJS += hal5_rng.o hal5_systick.o
HAL5_OBJS += hal5_watchdog.o

ELF_OBJS := startup_stm32h5.o syscalls.o
ELF_OBJS += main.o bsp_nucleo_h563zi.o

# compiler
CC := arm-none-eabi-gcc
AR := arm-none-eabi-ar
RM := rm -f

all: clean hal5.a hal5.elf

clean:
	$(RM) hal5.a
	$(RM) hal5.elf
	$(RM) *.o

%.o: %.c | cmsis cmsis_device_h5
	$(CC) $(CFLAGS) -c -o $@ $<

cmsis:
	git clone --depth 1 -b 5.9.0 https://github.com/ARM-software/CMSIS_5 cmsis

cmsis_device_h5:
	git clone --depth 1 -b v1.1.0 https://github.com/STMicroelectronics/cmsis_device_h5 cmsis_device_h5

hal5.a: $(HAL5_OBJS)
	echo $(CC)
	$(AR) rcs $@ $(HAL5_OBJS)

hal5.elf: $(ELF_OBJS) hal5.a
	$(CC) -T"startup.ld" $(LDFLAGS) -o $@ $(ELF_OBJS) hal5.a

# programmer
STM32PRG ?= STM32_Programmer_CLI --verbosity 1 -c port=swd mode=HOTPLUG speed=Reliable

flash: hal5.elf
	$(STM32PRG) --write $<
	$(STM32PRG) -hardRst

erase:
	$(STM32PRG) --erase all

reset:
	$(STM32PRG) -hardRst
