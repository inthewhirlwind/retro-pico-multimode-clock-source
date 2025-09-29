/**
 * Reset Control Module for Multimode Clock Source
 * 
 * This module handles reset pulse generation and LED management.
 * Provides a clean interface for reset control that can be reused in other projects.
 */

#ifndef RESET_CONTROL_H
#define RESET_CONTROL_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

/**
 * Initialize reset control module
 */
void reset_control_init(void);

/**
 * Handle reset button press detection
 */
void handle_reset_button(void);

/**
 * Start a reset pulse sequence
 */
void start_reset_pulse(void);

/**
 * Update reset state machine (call regularly from main loop)
 */
void update_reset_state(void);

/**
 * Update reset LED indicators
 */
void update_reset_leds(void);

/**
 * Set reset output pin state
 * @param state true for HIGH (not resetting), false for LOW (resetting)
 */
void set_reset_output(bool state);

/**
 * Get reset active state
 * @return true if reset pulse is currently active
 */
bool get_reset_active(void);

/**
 * Get reset output state
 * @return true if reset output is HIGH, false if LOW
 */
bool get_reset_output_state(void);

#endif // RESET_CONTROL_H