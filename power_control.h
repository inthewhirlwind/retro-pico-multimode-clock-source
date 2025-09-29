/**
 * Power Control Module for Multimode Clock Source
 * 
 * This module handles power state management and LED indication.
 * Provides a clean interface for power control that can be reused in other projects.
 */

#ifndef POWER_CONTROL_H
#define POWER_CONTROL_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"

/**
 * Initialize power control module
 */
void power_control_init(void);

/**
 * Handle power button press detection
 */
void handle_power_button(void);

/**
 * Toggle power state (ON->OFF or OFF->ON)
 */
void toggle_power_state(void);

/**
 * Set power state explicitly
 * @param state true for ON, false for OFF
 */
void set_power_state(bool state);

/**
 * Update power LED indicator
 */
void update_power_led(void);

/**
 * Get current power state
 * @return true if power is ON, false if OFF
 */
bool get_power_state(void);

#endif // POWER_CONTROL_H