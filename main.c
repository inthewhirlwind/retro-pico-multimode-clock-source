/**
 * Multimode Clock Source for Raspberry Pi Pico
 * 
 * Features:
 * - Single Step Mode: Manual clock toggle with button
 * - Low-Frequency Mode: 1Hz-100Hz (20% pot) and 100Hz-100kHz (80% pot)  
 * - High-Frequency Mode: Fixed 1MHz output
 * - LED indicators for each mode
 * - UART output for status display
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/uart.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"

// Pin definitions
#define BUTTON_SINGLE_STEP  2   // Button 1: Single step mode
#define BUTTON_LOW_FREQ     3   // Button 2: Low frequency mode  
#define BUTTON_HIGH_FREQ    4   // Button 3: High frequency mode

#define LED_CLOCK_ACTIVITY  5   // Clock activity indicator
#define LED_SINGLE_STEP     6   // Single step mode indicator
#define LED_LOW_FREQ        7   // Low frequency mode indicator  
#define LED_HIGH_FREQ       8   // High frequency mode indicator

#define CLOCK_OUTPUT        9   // Main clock output pin
#define POTENTIOMETER_PIN   26  // ADC0 - Potentiometer input (GPIO 26)

// Clock modes
typedef enum {
    MODE_SINGLE_STEP,
    MODE_LOW_FREQ,
    MODE_HIGH_FREQ
} clock_mode_t;

// Global variables
static clock_mode_t current_mode = MODE_SINGLE_STEP;
static bool clock_state = false;
static uint32_t current_frequency = 0;
static bool single_step_active = false;

// Button debouncing
static uint32_t last_button_time[3] = {0, 0, 0};
static const uint32_t DEBOUNCE_DELAY_MS = 50;

// Timer for low frequency mode
static struct repeating_timer low_freq_timer;
static bool timer_active = false;

// Function prototypes
void init_gpio(void);
void init_adc(void);
void init_uart(void);
void handle_buttons(void);
bool button_pressed(uint pin, uint button_index);
void set_mode(clock_mode_t mode);
void update_leds(void);
void toggle_clock_output(void);
void set_clock_output(bool state);
void update_low_frequency(void);
void start_high_frequency(void);
void stop_high_frequency(void);
bool low_freq_timer_callback(struct repeating_timer *t);
void print_status(void);
uint32_t calculate_frequency_from_pot(uint16_t adc_value);

int main() {
    stdio_init_all();
    
    // Initialize hardware
    init_gpio();
    init_adc();
    init_uart();
    
    // Set initial mode
    set_mode(MODE_SINGLE_STEP);
    
    printf("Multimode Clock Source Starting...\n");
    print_status();
    
    while (true) {
        handle_buttons();
        
        // Update low frequency if in that mode
        if (current_mode == MODE_LOW_FREQ) {
            update_low_frequency();
        }
        
        sleep_ms(10); // Small delay to prevent excessive polling
    }
    
    return 0;
}

void init_gpio(void) {
    // Initialize buttons as inputs with pull-up
    gpio_init(BUTTON_SINGLE_STEP);
    gpio_set_dir(BUTTON_SINGLE_STEP, GPIO_IN);
    gpio_pull_up(BUTTON_SINGLE_STEP);
    
    gpio_init(BUTTON_LOW_FREQ);
    gpio_set_dir(BUTTON_LOW_FREQ, GPIO_IN);
    gpio_pull_up(BUTTON_LOW_FREQ);
    
    gpio_init(BUTTON_HIGH_FREQ);
    gpio_set_dir(BUTTON_HIGH_FREQ, GPIO_IN);
    gpio_pull_up(BUTTON_HIGH_FREQ);
    
    // Initialize LEDs as outputs
    gpio_init(LED_CLOCK_ACTIVITY);
    gpio_set_dir(LED_CLOCK_ACTIVITY, GPIO_OUT);
    gpio_put(LED_CLOCK_ACTIVITY, 0);
    
    gpio_init(LED_SINGLE_STEP);
    gpio_set_dir(LED_SINGLE_STEP, GPIO_OUT);
    gpio_put(LED_SINGLE_STEP, 0);
    
    gpio_init(LED_LOW_FREQ);
    gpio_set_dir(LED_LOW_FREQ, GPIO_OUT);
    gpio_put(LED_LOW_FREQ, 0);
    
    gpio_init(LED_HIGH_FREQ);
    gpio_set_dir(LED_HIGH_FREQ, GPIO_OUT);
    gpio_put(LED_HIGH_FREQ, 0);
    
    // Initialize clock output
    gpio_init(CLOCK_OUTPUT);
    gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
    gpio_put(CLOCK_OUTPUT, 0);
}

void init_adc(void) {
    adc_init();
    adc_gpio_init(POTENTIOMETER_PIN);
    adc_select_input(0); // ADC0 corresponds to GPIO 26
}

void init_uart(void) {
    // UART is already initialized by stdio_init_all()
    // This function is here for completeness and future expansion
}

void handle_buttons(void) {
    if (button_pressed(BUTTON_SINGLE_STEP, 0)) {
        if (current_mode == MODE_SINGLE_STEP) {
            // Toggle clock in single step mode
            toggle_clock_output();
            single_step_active = true;
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

bool button_pressed(uint pin, uint button_index) {
    bool pressed = !gpio_get(pin); // Active low with pull-up
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (pressed && (current_time - last_button_time[button_index] > DEBOUNCE_DELAY_MS)) {
        last_button_time[button_index] = current_time;
        return true;
    }
    
    return false;
}

void set_mode(clock_mode_t mode) {
    // Stop any active timers or PWM
    if (timer_active) {
        cancel_repeating_timer(&low_freq_timer);
        timer_active = false;
    }
    stop_high_frequency();
    
    current_mode = mode;
    single_step_active = false;
    set_clock_output(false);
    
    switch (mode) {
        case MODE_SINGLE_STEP:
            current_frequency = 0;
            break;
            
        case MODE_LOW_FREQ:
            update_low_frequency();
            break;
            
        case MODE_HIGH_FREQ:
            current_frequency = 1000000; // 1MHz
            start_high_frequency();
            break;
    }
    
    update_leds();
    print_status();
}

void update_leds(void) {
    // Clear all mode LEDs
    gpio_put(LED_SINGLE_STEP, 0);
    gpio_put(LED_LOW_FREQ, 0);
    gpio_put(LED_HIGH_FREQ, 0);
    
    // Set the appropriate mode LED
    switch (current_mode) {
        case MODE_SINGLE_STEP:
            gpio_put(LED_SINGLE_STEP, 1);
            break;
        case MODE_LOW_FREQ:
            gpio_put(LED_LOW_FREQ, 1);
            break;
        case MODE_HIGH_FREQ:
            gpio_put(LED_HIGH_FREQ, 1);
            break;
    }
    
    // Update clock activity LED
    gpio_put(LED_CLOCK_ACTIVITY, clock_state);
}

void toggle_clock_output(void) {
    clock_state = !clock_state;
    set_clock_output(clock_state);
    update_leds();
}

void set_clock_output(bool state) {
    clock_state = state;
    gpio_put(CLOCK_OUTPUT, state);
    gpio_put(LED_CLOCK_ACTIVITY, state);
}

void update_low_frequency(void) {
    uint16_t adc_value = adc_read();
    uint32_t new_frequency = calculate_frequency_from_pot(adc_value);
    
    if (new_frequency != current_frequency) {
        current_frequency = new_frequency;
        
        // Stop current timer
        if (timer_active) {
            cancel_repeating_timer(&low_freq_timer);
            timer_active = false;
        }
        
        // Start new timer if frequency > 0
        if (current_frequency > 0) {
            uint32_t period_us = 500000 / current_frequency; // Half period in microseconds
            add_repeating_timer_us(-period_us, low_freq_timer_callback, NULL, &low_freq_timer);
            timer_active = true;
        }
        
        print_status();
    }
}

uint32_t calculate_frequency_from_pot(uint16_t adc_value) {
    // ADC is 12-bit, so values range from 0 to 4095
    float pot_position = (float)adc_value / 4095.0f;
    
    if (pot_position <= 0.2f) {
        // First 20%: 1Hz to 100Hz linear
        return (uint32_t)(1 + (pot_position / 0.2f) * 99);
    } else {
        // Remaining 80%: 100Hz to 100kHz
        float remaining_position = (pot_position - 0.2f) / 0.8f;
        return (uint32_t)(100 + remaining_position * 99900);
    }
}

bool low_freq_timer_callback(struct repeating_timer *t) {
    toggle_clock_output();
    return true; // Continue repeating
}

void start_high_frequency(void) {
    // Use PWM to generate 1MHz square wave
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    
    // Set PWM frequency to 1MHz with 50% duty cycle
    // System clock is typically 125MHz
    // For 1MHz: divider = 125, wrap = 1 (gives 125MHz / 125 / 1 = 1MHz)
    pwm_set_clkdiv(slice_num, 125.0f);
    pwm_set_wrap(slice_num, 1);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 1); // 50% duty cycle
    
    // Enable PWM
    pwm_set_enabled(slice_num, true);
    
    // Set clock activity LED on
    gpio_put(LED_CLOCK_ACTIVITY, 1);
}

void stop_high_frequency(void) {
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    pwm_set_enabled(slice_num, false);
    gpio_put(LED_CLOCK_ACTIVITY, 0);
}

void print_status(void) {
    printf("\n=== Clock Source Status ===\n");
    
    switch (current_mode) {
        case MODE_SINGLE_STEP:
            printf("Mode: Single Step\n");
            printf("Status: %s\n", single_step_active ? "Active" : "Waiting for button press");
            break;
            
        case MODE_LOW_FREQ:
            printf("Mode: Low Frequency\n");
            printf("Frequency: %lu Hz\n", current_frequency);
            break;
            
        case MODE_HIGH_FREQ:
            printf("Mode: High Frequency\n");
            printf("Frequency: %lu Hz (1MHz)\n", current_frequency);
            break;
    }
    
    printf("Clock State: %s\n", clock_state ? "HIGH" : "LOW");
    printf("===========================\n\n");
}