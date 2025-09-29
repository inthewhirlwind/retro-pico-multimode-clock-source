/**
 * Reset Control Module for Multimode Clock Source
 */

#include "reset_control.h"
#include "config.h"
#include <stdio.h>

// Reset control state variables
static bool reset_active = false;
static bool reset_output_state = true; // Reset output is normally high
static uint32_t reset_cycle_count = 0;
static uint32_t reset_start_time = 0;
static uint32_t reset_high_led_timer = 0;
static bool reset_waiting_for_edge = false; // For Mode 1 edge detection
static bool last_clock_state_for_reset = false;

// Button debouncing for reset button
static uint32_t last_reset_button_time = 0;

// External function declarations
extern clock_mode_t get_current_mode(void);
extern bool get_clock_state(void);
extern uint32_t get_current_frequency(void);
extern uint32_t get_uart_set_frequency(void);

void reset_control_init(void) {
    reset_active = false;
    reset_output_state = true;
    reset_cycle_count = 0;
    reset_start_time = 0;
    reset_high_led_timer = 0;
    reset_waiting_for_edge = false;
    last_clock_state_for_reset = false;
    last_reset_button_time = 0;
}

void handle_reset_button(void) {
    // Check for reset button press (positive edge triggered, active low with pull-up)
    bool pressed = !gpio_get(BUTTON_RESET);
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (pressed && (current_time - last_reset_button_time > DEBOUNCE_DELAY_MS)) {
        last_reset_button_time = current_time;
        if (!reset_active) {
            start_reset_pulse();
            printf("Reset pulse initiated\n");
        }
    }
}

void start_reset_pulse(void) {
    reset_active = true;
    reset_cycle_count = 0;
    reset_start_time = to_ms_since_boot(get_absolute_time());
    reset_waiting_for_edge = (get_current_mode() == MODE_SINGLE_STEP); // Mode 1 needs edge detection
    last_clock_state_for_reset = get_clock_state();
    set_reset_output(false); // Start reset pulse (low)
    printf("Reset pulse started, mode: %d\n", get_current_mode() + 1);
}

void update_reset_state(void) {
    if (!reset_active) return;
    
    clock_mode_t current_mode = get_current_mode();
    
    if (current_mode == MODE_SINGLE_STEP) {
        // Mode 1: Track clock low-to-high transitions
        bool current_clock_state = get_clock_state();
        if (reset_waiting_for_edge && !last_clock_state_for_reset && current_clock_state) {
            // Detected low-to-high transition
            reset_cycle_count++;
            printf("Reset cycle %d/6 (Mode 1)\n", reset_cycle_count);
            
            if (reset_cycle_count >= RESET_CYCLES) {
                set_reset_output(true); // End reset pulse
                reset_active = false;
                reset_high_led_timer = to_ms_since_boot(get_absolute_time());
                printf("Reset pulse complete (Mode 1)\n");
            }
        }
        last_clock_state_for_reset = current_clock_state;
    } else {
        // Modes 2, 3, 4: Count actual clock cycles using timing
        uint32_t elapsed_ms = to_ms_since_boot(get_absolute_time()) - reset_start_time;
        uint32_t required_ms = 0;
        
        // Calculate required time based on current frequency
        if (current_mode == MODE_LOW_FREQ && get_current_frequency() > 0) {
            // For low frequency mode, use current frequency
            required_ms = (RESET_CYCLES * 1000) / get_current_frequency();
        } else if (current_mode == MODE_HIGH_FREQ) {
            // For high frequency mode, use fixed 1MHz
            required_ms = (RESET_CYCLES * 1000) / HIGH_FREQ_OUTPUT;
            if (required_ms == 0) required_ms = 1; // Minimum 1ms for visibility
        } else if (current_mode == MODE_UART_CONTROL && get_uart_set_frequency() > 0) {
            // For UART control mode, use set frequency
            required_ms = (RESET_CYCLES * 1000) / get_uart_set_frequency();
        } else {
            // Fallback for any undefined states
            required_ms = 60; // Default 60ms (approximately 100Hz for 6 cycles)
        }
        
        // Ensure minimum time for visibility (at least 10ms)
        if (required_ms < 10) required_ms = 10;
        
        if (elapsed_ms >= required_ms) {
            set_reset_output(true); // End reset pulse
            reset_active = false;
            reset_high_led_timer = to_ms_since_boot(get_absolute_time());
            printf("Reset pulse complete (Mode %d, %dms)\n", current_mode + 1, elapsed_ms);
        }
    }
}

void update_reset_leds(void) {
    // LED_RESET_LOW: On when reset output is low
    gpio_put(LED_RESET_LOW, !reset_output_state);
    
    // LED_RESET_HIGH: On for 250ms when reset returns to high
    if (reset_high_led_timer > 0) {
        uint32_t current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - reset_high_led_timer < RESET_HIGH_LED_MS) {
            gpio_put(LED_RESET_HIGH, 1);
        } else {
            gpio_put(LED_RESET_HIGH, 0);
            reset_high_led_timer = 0; // Clear timer
        }
    } else {
        gpio_put(LED_RESET_HIGH, 0);
    }
}

void set_reset_output(bool state) {
    reset_output_state = state;
    gpio_put(RESET_OUTPUT, state);
}

bool get_reset_active(void) {
    return reset_active;
}

bool get_reset_output_state(void) {
    return reset_output_state;
}