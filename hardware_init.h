/**
 * Hardware Initialization Module for Multimode Clock Source
 * 
 * This module handles initialization of GPIO, ADC, and UART peripherals.
 * Provides a clean interface for hardware setup that can be reused in other projects.
 */

#ifndef HARDWARE_INIT_H
#define HARDWARE_INIT_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"

/**
 * Initialize all GPIO pins for buttons, LEDs, and outputs
 */
void init_gpio(void);

/**
 * Initialize ADC for potentiometer reading
 */
void init_adc(void);

/**
 * Initialize primary UART (USB CDC)
 */
void init_uart(void);

/**
 * Initialize secondary UART (hardware UART1 on GPIO 16/17)
 */
void init_second_uart(void);

/**
 * Initialize all hardware components in the correct order
 * This is the main entry point for hardware initialization
 */
void init_all_hardware(void);

#endif // HARDWARE_INIT_H