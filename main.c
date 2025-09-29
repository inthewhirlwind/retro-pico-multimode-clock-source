/**
 * Multimode Clock Source for Raspberry Pi Pico - Refactored Version
 * 
 * Features:
 * - Single Step Mode: Manual clock toggle with button
 * - Low-Frequency Mode: 1Hz-100Hz (20% pot) and 100Hz-100kHz (80% pot)  
 * - High-Frequency Mode: Fixed 1MHz output
 * - UART Control Mode: UART-controlled frequency from 10Hz to 1MHz
 * - LED indicators for each mode
 * - UART output for status display
 * 
 * This refactored version uses modular components for better code organization
 * and reusability. Each module handles a specific aspect of functionality.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "config.h"

// Module includes
#include "hardware_init.h"
#include "button_handler.h"
#include "clock_generator.h"
#include "uart_control.h"
#include "reset_control.h"
#include "power_control.h"
#include "status_display.h"

// Global mode management
void set_mode(clock_mode_t mode);

int main() {
    // Initialize all hardware components
    init_all_hardware();
    
    // Initialize all modules
    button_handler_init();
    clock_generator_init();
    uart_control_init();
    reset_control_init();
    power_control_init();
    status_display_init();
    
    // Set initial mode
    set_mode(MODE_SINGLE_STEP);
    
    printf("Multimode Clock Source Starting...\n");
    uart_puts(uart1, "Multimode Clock Source Starting...\n");
    printf("Press and hold any button for 3 seconds to enter UART Control Mode\n");
    print_status();
    
    uint32_t button_hold_start = 0;
    bool button_held = false;
    
    while (true) {
        clock_mode_t current_mode = get_current_mode();
        
        // Check for button hold to enter UART mode (only if not in UART mode)
        if (current_mode != MODE_UART_CONTROL) {
            if (any_button_pressed()) {
                if (!button_held) {
                    button_hold_start = to_ms_since_boot(get_absolute_time());
                    button_held = true;
                }
                
                // Check if held for 3 seconds
                if (to_ms_since_boot(get_absolute_time()) - button_hold_start > 3000) {
                    printf("Entering UART Control Mode\n");
                    set_mode(MODE_UART_CONTROL);
                    button_held = false;
                }
            } else {
                button_held = false;
            }
        }
        
        // Handle mode-specific processing
        if (current_mode == MODE_UART_CONTROL) {
            handle_uart_control();
        } else if (current_mode == MODE_LOW_FREQ) {
            update_low_frequency();
        } else if (current_mode != MODE_UART_CONTROL && !button_held) {
            // Only handle normal button presses when not holding for UART mode
            handle_buttons();
        }
        
        // Handle reset functionality (independent of mode)
        handle_reset_button();
        update_reset_state();
        update_reset_leds();
        
        // Handle power functionality (independent of mode)
        handle_power_button();
        update_power_led();
        
        sleep_ms(UPDATE_INTERVAL_MS); // Small delay to prevent excessive polling
    }
    
    return 0;
}

void set_mode(clock_mode_t mode) {
    // Stop all active clock generation
    stop_all_clock_generation();
    
    // Reset UART control state when leaving UART mode
    if (get_current_mode() == MODE_UART_CONTROL && mode != MODE_UART_CONTROL) {
        reset_uart_control_state();
    }
    
    // Update mode state
    set_current_mode(mode);
    set_single_step_active(false);
    set_clock_output(false);
    
    switch (mode) {
        case MODE_SINGLE_STEP:
            set_current_frequency(0);
            break;
            
        case MODE_LOW_FREQ:
            update_low_frequency();
            break;
            
        case MODE_HIGH_FREQ:
            set_current_frequency(HIGH_FREQ_OUTPUT); // 1MHz
            start_high_frequency();
            break;
            
        case MODE_UART_CONTROL:
            set_current_frequency(0);
            set_uart_menu_timeout(UART_MENU_TIMEOUT_MS);
            show_uart_menu();
            break;
    }
    
    update_leds();
    print_status();
}