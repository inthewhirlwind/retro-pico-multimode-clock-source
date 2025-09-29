/**
 * Status Display Module for Multimode Clock Source
 */

#include "status_display.h"
#include "config.h"
#include "hardware/gpio.h"
#include <stdio.h>

// External function declarations
extern clock_mode_t get_current_mode(void);
extern bool get_single_step_active(void);
extern uint32_t get_current_frequency(void);
extern bool get_uart_clock_running(void);
extern uint32_t get_uart_set_frequency(void);
extern bool get_uart_pwm_active(void);
extern bool get_clock_state(void);
extern bool get_power_state(void);

void status_display_init(void) {
    // No specific initialization needed for this module
}

void print_status_to_uart1(void) {
    const char* status_header = "\n=== Clock Source Status ===\n";
    const char* status_footer = "===========================\n\n";
    
    // Send header
    uart_puts(uart1, status_header);
    
    clock_mode_t current_mode = get_current_mode();
    
    switch (current_mode) {
        case MODE_SINGLE_STEP:
            uart_puts(uart1, "Mode: Single Step\n");
            if (get_single_step_active()) {
                uart_puts(uart1, "Status: Active\n");
            } else {
                uart_puts(uart1, "Status: Waiting for button press\n");
            }
            break;
            
        case MODE_LOW_FREQ:
            uart_puts(uart1, "Mode: Low Frequency\n");
            // Format frequency string
            char freq_str[32];
            snprintf(freq_str, sizeof(freq_str), "Frequency: %lu Hz\n", get_current_frequency());
            uart_puts(uart1, freq_str);
            break;
            
        case MODE_HIGH_FREQ:
            uart_puts(uart1, "Mode: High Frequency\n");
            char hfreq_str[32];
            snprintf(hfreq_str, sizeof(hfreq_str), "Frequency: %lu Hz (1MHz)\n", get_current_frequency());
            uart_puts(uart1, hfreq_str);
            break;
            
        case MODE_UART_CONTROL:
            uart_puts(uart1, "Mode: UART Control\n");
            if (get_uart_clock_running() && get_uart_set_frequency() > 0) {
                char ufreq_str[32];
                snprintf(ufreq_str, sizeof(ufreq_str), "Frequency: %lu Hz\n", get_uart_set_frequency());
                uart_puts(uart1, ufreq_str);
                uart_puts(uart1, "Status: Running\n");
            } else {
                uart_puts(uart1, "Status: Stopped\n");
            }
            break;
    }
    
    // Clock state
    if (current_mode == MODE_UART_CONTROL && get_uart_pwm_active()) {
        uart_puts(uart1, "Clock State: PWM Active\n");
    } else if (current_mode == MODE_HIGH_FREQ) {
        uart_puts(uart1, "Clock State: PWM Active\n");
    } else if (get_clock_state()) {
        uart_puts(uart1, "Clock State: HIGH\n");
    } else {
        uart_puts(uart1, "Clock State: LOW\n");
    }
    
    // Power state
    if (get_power_state()) {
        uart_puts(uart1, "Power State: ON\n");
    } else {
        uart_puts(uart1, "Power State: OFF\n");
    }
    
    // Send footer
    uart_puts(uart1, status_footer);
}

void print_status(void) {
    printf("\n=== Clock Source Status ===\n");
    
    clock_mode_t current_mode = get_current_mode();
    
    switch (current_mode) {
        case MODE_SINGLE_STEP:
            printf("Mode: Single Step\n");
            printf("Status: %s\n", get_single_step_active() ? "Active" : "Waiting for button press");
            break;
            
        case MODE_LOW_FREQ:
            printf("Mode: Low Frequency\n");
            printf("Frequency: %lu Hz\n", get_current_frequency());
            break;
            
        case MODE_HIGH_FREQ:
            printf("Mode: High Frequency\n");
            printf("Frequency: %lu Hz (1MHz)\n", get_current_frequency());
            break;
            
        case MODE_UART_CONTROL:
            printf("Mode: UART Control\n");
            if (get_uart_clock_running() && get_uart_set_frequency() > 0) {
                printf("Frequency: %lu Hz\n", get_uart_set_frequency());
                printf("Status: Running\n");
            } else {
                printf("Status: Stopped\n");
            }
            break;
    }
    
    printf("Clock State: %s\n", 
           (current_mode == MODE_UART_CONTROL && get_uart_pwm_active()) ? "PWM Active" :
           (current_mode == MODE_HIGH_FREQ) ? "PWM Active" :
           (get_clock_state() ? "HIGH" : "LOW"));
    printf("Power State: %s\n", get_power_state() ? "ON" : "OFF");
    printf("===========================\n\n");
    
    // Also send status to second UART
    print_status_to_uart1();
}

void update_leds(void) {
    // Clear all mode LEDs
    gpio_put(LED_SINGLE_STEP, 0);
    gpio_put(LED_LOW_FREQ, 0);
    gpio_put(LED_HIGH_FREQ, 0);
    gpio_put(LED_UART_MODE, 0);
    
    // Set the appropriate mode LED
    clock_mode_t current_mode = get_current_mode();
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
        case MODE_UART_CONTROL:
            gpio_put(LED_UART_MODE, 1);
            break;
    }
    
    // Update clock activity LED
    gpio_put(LED_CLOCK_ACTIVITY, get_clock_state());
}