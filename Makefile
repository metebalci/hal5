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

DEVICE		= stm32h5
BOARD		= nucleo_h563zi
APP_OBJS	= main.o syscalls.o 
APP_OBJS	+= startup_$(DEVICE).o bsp_$(BOARD).o 
APP_OBJS	+= example_usb_device.o

# set fpu to soft, softfp or hard
# soft:   software fpu, soft abi
# softfp: hardware fpu, soft abi
# hard:   harwdare fpu, hard abi
fpu = soft

# compiler
CC = arm-none-eabi-gcc
AR = arm-none-eabi-ar
RM = rm -f
# programmer
STM32PRG = STM32_Programmer_CLI --verbosity 1 -c port=swd mode=HOTPLUG speed=Reliable

HAL5_OBJS	+= hal5.o hal5_cache.o hal5_console.o hal5_crs.o
HAL5_OBJS	+= hal5_flash.o hal5_gpio.o hal5_i2c.o hal5_lpuart.o
HAL5_OBJS	+= hal5_pwr.o hal5_rcc.o hal5_rcc_ck.o
HAL5_OBJS	+= hal5_rng.o hal5_systick.o
HAL5_OBJS	+= hal5_watchdog.o
HAL5_OBJS	+= hal5_assert.o
# USB
HAL5_OBJS	+= hal5_usb.o hal5_usb_device.o hal5_usb_device_ep0.o

# FLOATFLAGS based on fpu above
ifeq ($(fpu), softfp)
	FLOATFLAGS = -mfloat-abi=softfp -mfpu=fpv5-sp-d16
else ifeq ($(fpu), hard)
	FLOATFLAGS = -mfloat-abi=hard -mfpu=fpv5-sp-d16
else
	FLOATFLAGS = -mfloat-abi=soft
endif

CFLAGS = -std=gnu11
CFLAGS += -mcpu=cortex-m33 -mthumb
CFLAGS += -O2 -ffunction-sections -fdata-sections
CFLAGS += $(FLOATFLAGS)
CFLAGS += -I. -Icmsis/CMSIS/Core/Include -Icmsis_device_h5/Include -DSTM32H563xx
CFLAGS += --specs=nano.specs
CFLAGS += -Wall #-Werror
CFLAGS += -Wno-unused-variable -Wno-unused-function 
CFLAGS += -fmax-errors=5

LDFLAGS = -mcpu=cortex-m33 -mthumb 
LDFLAGS += $(FLOATFLAGS)
LDFLAGS += --specs=nosys.specs 
LDFLAGS += -Wl,--gc-sections
LDFLAGS += -static
LDFLAGS += --specs=nano.specs
LDFLAGS += -Wl,--start-group -lc -lm -Wl,--end-group

LIB_NAME	= hal5.a
FW_NAME		= example.elf

DESCRIPTORS	= hal5_usb_device_descriptors
APP_OBJS	+= $(DESCRIPTORS).o

# run cmsis and cmsis_device_h5 only if the directories do not exist
all: clean lib fw

lib: $(LIB_NAME)
fw: $(FW_NAME)

clean:
	$(RM) $(FW_NAME) 
	$(RM) $(LIB_NAME) 
	$(RM) $(DESCRIPTORS).c
	$(RM) *.o

$(DESCRIPTORS).c: descriptors.py
	./create_descriptors.py > $(DESCRIPTORS).c

%.o: %.c | cmsis cmsis_device_h5
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB_NAME): $(HAL5_OBJS)
	$(AR) rcs $@ $(HAL5_OBJS)

$(FW_NAME): $(APP_OBJS) $(LIB_NAME)
	$(CC) -T"startup.ld" $(LDFLAGS) -o $@ $(APP_OBJS) $(LIB_NAME)

flash: $(FW_NAME)
	$(STM32PRG) --write $<
	$(STM32PRG) -hardRst

erase:
	$(STM32PRG) --erase all

reset:
	$(STM32PRG) -hardRst

cmsis:
	git clone --depth 1 -b 5.9.0 https://github.com/ARM-software/CMSIS_5 $@

cmsis_device_h5:
	git clone --depth 1 -b v1.1.0 https://github.com/STMicroelectronics/cmsis_device_h5 $@

fixlicenses:
	reuse annotate --style=c --template=hal5 --merge-copyrights --license=Apache-2.0 --copyright="Mete Balci" --year 2023 *.c *.h *.ld
	reuse annotate --style=python --template=hash --merge-copyrights --license=Apache-2.0 --copyright="Mete Balci" --year 2023 *.py
