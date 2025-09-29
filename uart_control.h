/**
 * UART Control Module for Multimode Clock Source
 * 
 * This module handles UART command processing, frequency control via UART,
 * and PWM generation. Provides a clean interface for UART control that
 * can be reused in other projects.
 */

#ifndef UART_CONTROL_H
#define UART_CONTROL_H

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

/**
 * Initialize UART control module
 */
void uart_control_init(void);

/**
 * Handle UART control mode processing
 * Checks for button presses, timeouts, and processes UART commands
 */
void handle_uart_control(void);

/**
 * Show UART command menu
 */
void show_uart_menu(void);

/**
 * Process a UART command string
 * @param cmd Command string to process
 */
void process_uart_command(const char* cmd);

/**
 * Start UART-controlled frequency generation
 * @param frequency Frequency in Hz (10Hz to 1MHz)
 */
void start_uart_frequency(uint32_t frequency);

/**
 * Stop UART-controlled frequency generation
 */
void stop_uart_frequency(void);

/**
 * Start UART PWM output
 * @param frequency Frequency in Hz for PWM output
 */
void start_uart_pwm(uint32_t frequency);

/**
 * Stop UART PWM output
 */
void stop_uart_pwm(void);

/**
 * Get UART clock running state
 * @return true if UART clock is running
 */
bool get_uart_clock_running(void);

/**
 * Get UART set frequency
 * @return Currently set UART frequency in Hz
 */
uint32_t get_uart_set_frequency(void);

/**
 * Get UART PWM active state
 * @return true if UART PWM is active
 */
bool get_uart_pwm_active(void);

/**
 * Set UART menu timeout
 * @param timeout_ms Timeout in milliseconds from now
 */
void set_uart_menu_timeout(uint32_t timeout_ms);

/**
 * Reset UART control state (for mode switching)
 */
void reset_uart_control_state(void);

#endif // UART_CONTROL_H