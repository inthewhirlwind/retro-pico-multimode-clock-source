/**
 * Clock Generator Module for Multimode Clock Source
 */

#include "clock_generator.h"
#include "config.h"
#include "hardware/gpio.h"

// Static variables for clock generation
static bool clock_state = false;
static uint32_t current_frequency = 0;
static bool single_step_active = false;

// Timer for low frequency mode
static struct repeating_timer low_freq_timer;
static bool timer_active = false;

void clock_generator_init(void) {
    clock_state = false;
    current_frequency = 0;
    single_step_active = false;
    timer_active = false;
}

void toggle_clock_output(void) {
    clock_state = !clock_state;
    gpio_put(CLOCK_OUTPUT, clock_state);
    gpio_put(LED_CLOCK_ACTIVITY, clock_state);
}

void set_clock_output(bool state) {
    clock_state = state;
    gpio_put(CLOCK_OUTPUT, state);
    gpio_put(LED_CLOCK_ACTIVITY, state);
}

bool get_clock_state(void) {
    return clock_state;
}

void update_low_frequency(void) {
    // Read potentiometer value
    uint16_t adc_value = adc_read();
    uint32_t new_frequency = calculate_frequency_from_pot(adc_value);
    
    if (new_frequency != current_frequency) {
        current_frequency = new_frequency;
        
        // Stop existing timer if active
        if (timer_active) {
            cancel_repeating_timer(&low_freq_timer);
            timer_active = false;
        }
        
        // Start new timer with calculated period
        if (current_frequency > 0) {
            uint32_t period_us = 1000000 / (current_frequency * 2); // Divide by 2 for 50% duty cycle
            if (add_repeating_timer_us(-period_us, low_freq_timer_callback, NULL, &low_freq_timer)) {
                timer_active = true;
            }
        }
    }
}

uint32_t calculate_frequency_from_pot(uint16_t adc_value) {
    // Scale ADC value (0-4095) to frequency range
    // First 20% of pot range: 1Hz to 100Hz
    // Remaining 80% of pot range: 100Hz to 100kHz
    
    if (adc_value <= 819) { // First 20% (4095 * 0.2 = 819)
        // Linear scaling from 1Hz to 100Hz
        return MIN_LOW_FREQ + ((adc_value * (MAX_LOW_FREQ_RANGE1 - MIN_LOW_FREQ)) / 819);
    } else {
        // Linear scaling from 100Hz to 100kHz
        uint16_t scaled_adc = adc_value - 819; // 0 to 3276
        return MAX_LOW_FREQ_RANGE1 + ((scaled_adc * (MAX_LOW_FREQ_RANGE2 - MAX_LOW_FREQ_RANGE1)) / 3276);
    }
}

bool low_freq_timer_callback(struct repeating_timer *t) {
    toggle_clock_output();
    return true; // Continue timer
}

void start_high_frequency(void) {
    // Set up PWM for 1MHz output with 50% duty cycle
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    
    // Configure PWM
    pwm_config config = pwm_get_default_config();
    
    // Calculate divider for 1MHz output
    // System clock is typically 125MHz, so we need divider of 125 for 1MHz
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, 1); // Wrap at 1 for 50% duty cycle
    
    pwm_init(slice_num, &config, true);
    
    // Set 50% duty cycle
    pwm_set_gpio_level(CLOCK_OUTPUT, 1);
    
    // Set function to PWM
    gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_PWM);
}

void stop_high_frequency(void) {
    // Stop PWM and return GPIO to normal function
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    pwm_set_enabled(slice_num, false);
    
    // Return GPIO to normal output function
    gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_SIO);
    gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
    set_clock_output(false);
}

void stop_all_clock_generation(void) {
    // Stop timer if active
    if (timer_active) {
        cancel_repeating_timer(&low_freq_timer);
        timer_active = false;
    }
    
    // Stop high frequency PWM
    stop_high_frequency();
    
    // Reset clock output
    set_clock_output(false);
}

uint32_t get_current_frequency(void) {
    return current_frequency;
}

void set_current_frequency(uint32_t frequency) {
    current_frequency = frequency;
}

bool get_single_step_active(void) {
    return single_step_active;
}

void set_single_step_active(bool active) {
    single_step_active = active;
}