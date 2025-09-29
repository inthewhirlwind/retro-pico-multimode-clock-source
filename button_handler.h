/**
 * Button Handler Module for Multimode Clock Source
 * 
 * This module handles button debouncing and mode switching logic.
 * Provides a clean interface for button handling that can be reused in other projects.
 */

#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

// Forward declaration of clock_mode_t (defined in main module)
typedef enum {
    MODE_SINGLE_STEP,
    MODE_LOW_FREQ,
    MODE_HIGH_FREQ,
    MODE_UART_CONTROL
} clock_mode_t;

/**
 * Initialize button handler module
 */
void button_handler_init(void);

/**
 * Check if a button is pressed with debouncing
 * @param pin GPIO pin number
 * @param button_index Index for debounce timing (0-4)
 * @return true if button was pressed (debounced)
 */
bool button_pressed(uint pin, uint button_index);

/**
 * Handle mode switching button presses
 * Updates current mode based on button presses
 */
void handle_buttons(void);

/**
 * Check if any button is currently pressed (for UART mode entry)
 * @return true if any mode button is currently pressed
 */
bool any_button_pressed(void);

/**
 * Get current mode
 * @return current clock mode
 */
clock_mode_t get_current_mode(void);

/**
 * Set current mode (used by mode switching logic)
 * @param mode New clock mode to set
 */
void set_current_mode(clock_mode_t mode);

/**
 * Get previous mode
 * @return previous clock mode
 */
clock_mode_t get_previous_mode(void);

#endif // BUTTON_HANDLER_H