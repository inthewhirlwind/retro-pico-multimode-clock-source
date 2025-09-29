/**
 * Status Display Module for Multimode Clock Source
 * 
 * This module handles status output to UART and LED management.
 * Provides a clean interface for status display that can be reused in other projects.
 */

#ifndef STATUS_DISPLAY_H
#define STATUS_DISPLAY_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "button_handler.h"

/**
 * Initialize status display module
 */
void status_display_init(void);

/**
 * Print status to primary UART (USB CDC)
 */
void print_status(void);

/**
 * Print status to secondary UART (GPIO 16/17)
 */
void print_status_to_uart1(void);

/**
 * Update mode LEDs based on current mode
 */
void update_leds(void);

#endif // STATUS_DISPLAY_H