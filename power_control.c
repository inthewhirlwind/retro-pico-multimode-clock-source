/**
 * Power Control Module for Multimode Clock Source
 */

#include "power_control.h"
#include "config.h"
#include <stdio.h>

// Power control state variables
static bool power_state = false; // false = OFF (default), true = ON

// Button debouncing for power button
static uint32_t last_power_button_time = 0;

// External function declaration
extern void set_mode(clock_mode_t mode);

void power_control_init(void) {
    power_state = false;
    last_power_button_time = 0;
}

void handle_power_button(void) {
    // Check for power button press (positive edge triggered, active low with pull-up)
    bool pressed = !gpio_get(BUTTON_POWER);
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (pressed && (current_time - last_power_button_time > DEBOUNCE_DELAY_MS)) {
        last_power_button_time = current_time;
        toggle_power_state();
        printf("Power %s\n", power_state ? "ON" : "OFF");
    }
}

void toggle_power_state(void) {
    bool old_power_state = power_state;
    set_power_state(!power_state);
    
    // If power just turned ON (OFF->ON transition), switch to Mode 1
    if (!old_power_state && power_state) {
        set_mode(MODE_SINGLE_STEP);
        printf("Power ON - automatically switched to Mode 1 (Single Step)\n");
    }
}

void set_power_state(bool state) {
    power_state = state;
    // Power control is inverted: LOW = power ON, HIGH = power OFF
    gpio_put(POWER_OUTPUT, !state);
    update_power_led();
}

void update_power_led(void) {
    // Power LED is on when power is enabled
    gpio_put(LED_POWER_ON, power_state);
}

bool get_power_state(void) {
    return power_state;
}