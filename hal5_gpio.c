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

static GPIO_TypeDef* const gpio_ports[] = {
    GPIOA,
    GPIOB,
    GPIOC,
    GPIOD,
    GPIOE,
    GPIOF,
    GPIOG,
    GPIOH,
    GPIOI
};

#define GPIO_PIN_TO_PORT_INDEX(x) ((x >> 8) & 0xFF)
#define GPIO_PIN_TO_PIN_NUMBER(x) (x & 0xFF)

static void hal5_gpio_configure(
        hal5_gpio_pin_t pin,
        hal5_gpio_mode_t mode,
        hal5_gpio_output_speed_t output_speed,
        hal5_gpio_af_t af)
{
    const uint32_t port_index   = GPIO_PIN_TO_PORT_INDEX(pin);
    GPIO_TypeDef* const port    = gpio_ports[port_index];
    const uint32_t pin_number   = GPIO_PIN_TO_PIN_NUMBER(pin);
    const uint32_t mask 		    = (1UL << pin_number);
    const uint32_t twobitmask   = (3UL << (pin_number<<1));

    hal5_rcc_enable_gpio_port_by_index(port_index);

    const uint32_t gpio_mode_type_pupd[][3] = {
        {0b00, 0xF, 0b00},
        {0b00, 0xF, 0b01},
        {0b00, 0xF, 0b10},
        {0b01, 0b1, 0b00},
        {0b01, 0b1, 0b01},
        {0b01, 0b1, 0b10},
        {0b01, 0b0, 0b00},
        {0b01, 0b0, 0b01},
        {0b01, 0b0, 0b10},
        {0b10, 0b1, 0b00},
        {0b10, 0b1, 0b01},
        {0b10, 0b1, 0b10},
        {0b10, 0b0, 0b00},
        {0b10, 0b0, 0b01},
        {0b10, 0b0, 0b10},
        {0b11, 0xF, 0b00},
    };

    const int output_mode = gpio_mode_type_pupd[mode][0];
    const int output_type = gpio_mode_type_pupd[mode][1];
    const int pupd = gpio_mode_type_pupd[mode][2];

    MODIFY_REG(port->MODER, twobitmask, output_mode << (pin_number<<1));

    // only for output and AF
    if ((output_mode == 0b01) || (output_mode == 0b10))
    {
        assert (output_type != 0xF);
        MODIFY_REG(port->OTYPER, mask, output_type << pin_number);

        assert (output_speed != hal5_gpio_output_speed_dont_care);

        uint32_t output_speed_bits;

        switch (output_speed)
        {
            case low_speed: output_speed_bits = 0b00; break;
            case medium_speed: output_speed_bits = 0b01; break;
            case high_speed: output_speed_bits = 0b10; break;
            case very_high_speed: output_speed_bits = 0b11; break;
            default: assert (false);
        }

        MODIFY_REG(port->OSPEEDR, twobitmask, output_speed_bits << (pin_number<<1));
    }

    MODIFY_REG(port->PUPDR, twobitmask, pupd << (pin_number<<1));

    // only for AF
    if (output_mode == 0b10) {

        assert (af != hal5_gpio_af_dont_care);

        uint32_t afsel_bits;
        switch (af)
        {
            case AF0:   afsel_bits = 0b0000; break;
            case AF1:   afsel_bits = 0b0001; break;
            case AF2:   afsel_bits = 0b0010; break;
            case AF3:   afsel_bits = 0b0011; break;
            case AF4:   afsel_bits = 0b0100; break;
            case AF5:   afsel_bits = 0b0101; break;
            case AF6:   afsel_bits = 0b0110; break;
            case AF7:   afsel_bits = 0b0111; break;
            case AF8:   afsel_bits = 0b1000; break;
            case AF9:   afsel_bits = 0b1001; break;
            case AF10:  afsel_bits = 0b1010; break;
            case AF11:  afsel_bits = 0b1011; break;
            case AF12:  afsel_bits = 0b1100; break;
            case AF13:  afsel_bits = 0b1101; break;
            case AF14:  afsel_bits = 0b1110; break;
            case AF15:  afsel_bits = 0b1111; break;
            default: assert (false);
        };

        // because there is a low and high AFR; AFRL and AFRH
        // if pin_number is >= 8, mask and value has to be created with pin_number-8
        if (pin_number >= 8) {
            const uint32_t fourbitmask = (0xFUL << ((pin_number-8)<<2));
            MODIFY_REG(port->AFR[1], fourbitmask, afsel_bits << ((pin_number-8)<<2));
        } else {
            const uint32_t fourbitmask = (0xFUL << (pin_number<<2));
            MODIFY_REG(port->AFR[0], fourbitmask, afsel_bits << (pin_number<<2));
        }

    }

}

void hal5_gpio_configure_as_input(
        hal5_gpio_pin_t pin,
        hal5_gpio_mode_t mode)
{
    hal5_gpio_configure(
            pin, 
            mode,
            hal5_gpio_output_speed_dont_care,
            hal5_gpio_af_dont_care);
}

void hal5_gpio_configure_as_output(
        hal5_gpio_pin_t pin,
        hal5_gpio_mode_t mode,
        hal5_gpio_output_speed_t output_speed)
{
    hal5_gpio_configure(
            pin, 
            mode,
            output_speed,
            hal5_gpio_af_dont_care);
}

void hal5_gpio_configure_as_af(
        hal5_gpio_pin_t pin,
        hal5_gpio_mode_t mode,
        hal5_gpio_output_speed_t speed,
        hal5_gpio_af_t af)
{
    hal5_gpio_configure(
            pin, 
            mode,
            speed,
            af);
}

void hal5_gpio_configure_as_analog(
        hal5_gpio_pin_t pin,
        hal5_gpio_mode_t mode)
{
    hal5_gpio_configure(
            pin, 
            mode, 
            hal5_gpio_output_speed_dont_care,
            hal5_gpio_af_dont_care);
}

void inline hal5_gpio_set(
        hal5_gpio_pin_t pin)
{
    const uint32_t port_index   = GPIO_PIN_TO_PORT_INDEX(pin);
    GPIO_TypeDef* const port    = gpio_ports[port_index];
    const uint32_t pin_number   = GPIO_PIN_TO_PIN_NUMBER(pin);

    port->BSRR = (1UL << pin_number);
}

void inline hal5_gpio_reset(
        hal5_gpio_pin_t pin)
{
    const uint32_t port_index   = GPIO_PIN_TO_PORT_INDEX(pin);
    GPIO_TypeDef* const port    = gpio_ports[port_index];
    const uint32_t pin_number   = GPIO_PIN_TO_PIN_NUMBER(pin);

    port->BSRR = (1UL << (pin_number+16));
}

bool inline hal5_gpio_get(
        hal5_gpio_pin_t pin)
{
    const uint32_t port_index   = GPIO_PIN_TO_PORT_INDEX(pin);
    GPIO_TypeDef* const port    = gpio_ports[port_index];
    const uint32_t pin_number   = GPIO_PIN_TO_PIN_NUMBER(pin);

    return (port->ODR & (1UL << pin_number));
}

void inline hal5_gpio_flip(
        hal5_gpio_pin_t pin)
{
    if (hal5_gpio_get(pin)) {
        hal5_gpio_reset(pin);
    } else {
        hal5_gpio_set(pin);
    }
}

// holds the callback function pointers for each exti
static void (*gpio_exti_callbacks[16])(void) = {NULL};

// macro for EXTI<N>_IRQHandlers
// each handler resets the pending bits
// calls the callback function if there is any
#define EXTI_IRQHandler(n) \
    void EXTI ## n ## _IRQHandler(void) \
{ \
    SET_BIT(EXTI->RPR1, 1 << n); \
    SET_BIT(EXTI->FPR1, 1 << n); \
    void (*fn)(void) = gpio_exti_callbacks[n]; \
    if (fn != NULL) fn(); \
}

    EXTI_IRQHandler(0)
    EXTI_IRQHandler(2)
    EXTI_IRQHandler(3)
    EXTI_IRQHandler(4)
    EXTI_IRQHandler(5)
    EXTI_IRQHandler(6)
    EXTI_IRQHandler(7)
    EXTI_IRQHandler(8)
    EXTI_IRQHandler(9)
    EXTI_IRQHandler(10)
    EXTI_IRQHandler(11)
    EXTI_IRQHandler(12)
    EXTI_IRQHandler(13)
    EXTI_IRQHandler(14)
EXTI_IRQHandler(15)

void hal5_gpio_configure_exti(
        hal5_gpio_pin_t pin,
        const bool rising_edge_trigger,
        const bool falling_edge_trigger,
        void (*callback)(void))
{
    const uint32_t port_index   = GPIO_PIN_TO_PORT_INDEX(pin);
    const uint32_t pin_number   = GPIO_PIN_TO_PIN_NUMBER(pin);
    // pins are assigned to inputs according to their number
    // e.g. all pins numbered 0 in all banks are in ext line 0
    const uint32_t input_line   = pin_number;

    // make sure the callback is not registered before
    // good for programming errors
    if (callback != NULL) 
    {
        assert (gpio_exti_callbacks[input_line] == NULL);
        gpio_exti_callbacks[input_line] = callback;
    }

    // EXTI lines are assigned per pin number 
    // e.g. EXTI0 is for all pins numbered 0 (PA0, PB0 ...)
    // there is a mux in front of an EXTI lines
    // so there are 16 muxes
    // mux inputs are GPIO pins
    // select mux here
    SET_BIT(EXTI->EXTICR[pin_number / 4], 
            port_index << ((pin_number % 4) * 8));

    if (rising_edge_trigger) {
        SET_BIT(EXTI->RTSR1, 1 << input_line);
    } else {
        CLEAR_BIT(EXTI->RTSR1, 1 << input_line);
    }

    if (falling_edge_trigger) {
        SET_BIT(EXTI->FTSR1, 1 << input_line);
    } else {
        CLEAR_BIT(EXTI->FTSR1, 1 << input_line);
    }

    // CPU wakeup with interrupt mask
    // although this is called wakeup
    // it is not only related to standby
    // interrupt has to be unmasked 
    // to get attention of CPU
    SET_BIT(EXTI->IMR1, 1 << input_line);

    IRQn_Type irq;
    switch (input_line)
    {
        case 0:   irq = EXTI0_IRQn; break;
        case 1:   irq = EXTI1_IRQn; break;
        case 2:   irq = EXTI2_IRQn; break;
        case 3:   irq = EXTI3_IRQn; break;
        case 4:   irq = EXTI4_IRQn; break;
        case 5:   irq = EXTI5_IRQn; break;
        case 6:   irq = EXTI6_IRQn; break;
        case 7:   irq = EXTI7_IRQn; break;
        case 8:   irq = EXTI8_IRQn; break;
        case 9:   irq = EXTI9_IRQn; break;
        case 10:  irq = EXTI10_IRQn; break;
        case 11:  irq = EXTI11_IRQn; break;
        case 12:  irq = EXTI12_IRQn; break;
        case 13:  irq = EXTI13_IRQn; break;
        case 14:  irq = EXTI14_IRQn; break;
        case 15:  irq = EXTI15_IRQn; break;
        default: assert (false);
    }

    NVIC_EnableIRQ(irq);
}

