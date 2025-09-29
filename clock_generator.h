/**
 * Clock Generator Module for Multimode Clock Source
 * 
 * This module handles all clock generation modes including single step,
 * low frequency, and high frequency modes. Provides a clean interface
 * for clock generation that can be reused in other projects.
 */

#ifndef CLOCK_GENERATOR_H
#define CLOCK_GENERATOR_H

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"

/**
 * Initialize clock generator module
 */
void clock_generator_init(void);

/**
 * Toggle clock output state (for single step mode)
 */
void toggle_clock_output(void);

/**
 * Set clock output to specific state
 * @param state true for HIGH, false for LOW
 */
void set_clock_output(bool state);

/**
 * Get current clock state
 * @return true if clock is HIGH, false if LOW
 */
bool get_clock_state(void);

/**
 * Update low frequency mode based on potentiometer reading
 */
void update_low_frequency(void);

/**
 * Calculate frequency from potentiometer ADC value
 * @param adc_value Raw ADC reading (0-4095)
 * @return Calculated frequency in Hz
 */
uint32_t calculate_frequency_from_pot(uint16_t adc_value);

/**
 * Timer callback for low frequency mode
 * @param t Pointer to repeating timer structure
 * @return true to continue timer, false to stop
 */
bool low_freq_timer_callback(struct repeating_timer *t);

/**
 * Start high frequency (1MHz) PWM output
 */
void start_high_frequency(void);

/**
 * Stop high frequency PWM output
 */
void stop_high_frequency(void);

/**
 * Stop all active timers and PWM
 */
void stop_all_clock_generation(void);

/**
 * Get current frequency
 * @return Current frequency in Hz (0 if stopped)
 */
uint32_t get_current_frequency(void);

/**
 * Set current frequency (for display purposes)
 * @param frequency Frequency in Hz
 */
void set_current_frequency(uint32_t frequency);

/**
 * Get single step active state
 * @return true if single step is active
 */
bool get_single_step_active(void);

/**
 * Set single step active state
 * @param active true to set active, false to clear
 */
void set_single_step_active(bool active);

#endif // CLOCK_GENERATOR_H