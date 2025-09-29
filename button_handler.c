/**
 * Button Handler Module for Multimode Clock Source
 */

#include "button_handler.h"
#include "config.h"

// Static variables for button debouncing
static uint32_t last_button_time[5] = {0, 0, 0, 0, 0}; // Added reset and power buttons

// Mode state variables
static clock_mode_t current_mode = MODE_SINGLE_STEP;
static clock_mode_t previous_mode = MODE_SINGLE_STEP;

// Forward declaration of external functions
extern void set_mode(clock_mode_t mode);
extern void toggle_clock_output(void);
extern void set_single_step_active(bool active);

void button_handler_init(void) {
    // Initialize button debounce times
    for (int i = 0; i < 5; i++) {
        last_button_time[i] = 0;
    }
}

bool button_pressed(uint pin, uint button_index) {
    bool pressed = !gpio_get(pin); // Active low with pull-up
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (pressed && (current_time - last_button_time[button_index] > DEBOUNCE_DELAY_MS)) {
        last_button_time[button_index] = current_time;
        return true;
    }
    
    return false;
}

void handle_buttons(void) {
    if (button_pressed(BUTTON_SINGLE_STEP, 0)) {
        if (current_mode == MODE_SINGLE_STEP) {
            // Toggle clock in single step mode
            toggle_clock_output();
            set_single_step_active(true);
        } else {
            // Switch to single step mode
            set_mode(MODE_SINGLE_STEP);
        }
    }
    
    if (button_pressed(BUTTON_LOW_FREQ, 1)) {
        set_mode(MODE_LOW_FREQ);
    }
    
    if (button_pressed(BUTTON_HIGH_FREQ, 2)) {
        set_mode(MODE_HIGH_FREQ);
    }
}

bool any_button_pressed(void) {
    return !gpio_get(BUTTON_SINGLE_STEP) ||
           !gpio_get(BUTTON_LOW_FREQ) ||
           !gpio_get(BUTTON_HIGH_FREQ);
}

clock_mode_t get_current_mode(void) {
    return current_mode;
}

void set_current_mode(clock_mode_t mode) {
    previous_mode = current_mode;
    current_mode = mode;
}

clock_mode_t get_previous_mode(void) {
    return previous_mode;
}