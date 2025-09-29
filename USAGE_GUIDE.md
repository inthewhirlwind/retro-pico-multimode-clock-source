# Module Usage Guide

This guide shows how to use the individual modules from this project in your own Raspberry Pi Pico projects.

## Quick Start

1. Copy the required module files (`.h` and `.c`) to your project
2. Update `config.h` with your pin definitions
3. Add modules to your `CMakeLists.txt`
4. Include and initialize modules in your code

## Module Usage Examples

### Hardware Initialization Module

**Files needed**: `hardware_init.h`, `hardware_init.c`, `config.h`

```c
#include "hardware_init.h"

int main() {
    // Initialize all hardware at once
    init_all_hardware();
    
    // Or initialize components individually
    init_gpio();      // Initialize all GPIO pins
    init_adc();       // Initialize ADC for potentiometer
    init_uart();      // Initialize primary UART
    init_second_uart(); // Initialize secondary UART
    
    // Your code here...
}
```

### Button Handler Module

**Files needed**: `button_handler.h`, `button_handler.c`, `config.h`

```c
#include "button_handler.h"

int main() {
    init_all_hardware();
    button_handler_init();
    
    while (true) {
        // Check for specific button press
        if (button_pressed(BUTTON_SINGLE_STEP, 0)) {
            printf("Button 1 pressed!\n");
        }
        
        // Check if any button is currently held
        if (any_button_pressed()) {
            printf("A button is being held\n");
        }
        
        // Handle mode switching (if using mode system)
        handle_buttons();
        
        sleep_ms(10);
    }
}
```

### Clock Generator Module

**Files needed**: `clock_generator.h`, `clock_generator.c`, `config.h`

```c
#include "clock_generator.h"

int main() {
    init_all_hardware();
    clock_generator_init();
    
    // Generate different types of clock signals
    
    // 1. Manual clock toggling
    toggle_clock_output();
    printf("Clock state: %s\n", get_clock_state() ? "HIGH" : "LOW");
    
    // 2. High frequency (1MHz) PWM
    start_high_frequency();
    sleep_ms(5000);
    stop_high_frequency();
    
    // 3. Variable frequency based on potentiometer
    while (true) {
        update_low_frequency();
        printf("Current frequency: %lu Hz\n", get_current_frequency());
        sleep_ms(100);
    }
}
```

### UART Control Module

**Files needed**: `uart_control.h`, `uart_control.c`, `config.h`

```c
#include "uart_control.h"

int main() {
    init_all_hardware();
    uart_control_init();
    
    // Show interactive menu
    show_uart_menu();
    
    while (true) {
        // Process UART commands
        handle_uart_control();
        
        // Or generate specific frequencies
        start_uart_frequency(1000); // 1kHz
        sleep_ms(5000);
        stop_uart_frequency();
        
        sleep_ms(10);
    }
}
```

### Reset Control Module

**Files needed**: `reset_control.h`, `reset_control.c`, `config.h`

```c
#include "reset_control.h"

int main() {
    init_all_hardware();
    reset_control_init();
    
    while (true) {
        // Handle reset button
        handle_reset_button();
        
        // Update reset state machine
        update_reset_state();
        update_reset_leds();
        
        // Trigger reset programmatically
        if (some_condition) {
            start_reset_pulse();
        }
        
        sleep_ms(10);
    }
}
```

### Power Control Module

**Files needed**: `power_control.h`, `power_control.c`, `config.h`

```c
#include "power_control.h"

int main() {
    init_all_hardware();
    power_control_init();
    
    while (true) {
        // Handle power button
        handle_power_button();
        update_power_led();
        
        // Control power programmatically
        set_power_state(true);  // Turn power ON
        printf("Power is %s\n", get_power_state() ? "ON" : "OFF");
        
        sleep_ms(10);
    }
}
```

### Status Display Module

**Files needed**: `status_display.h`, `status_display.c`, `config.h`

```c
#include "status_display.h"

int main() {
    init_all_hardware();
    status_display_init();
    
    while (true) {
        // Print status information
        print_status();        // To primary UART
        print_status_to_uart1(); // To secondary UART
        
        // Update mode LEDs
        update_leds();
        
        sleep_ms(1000);
    }
}
```

## Complete Example: Simple Clock Generator

Here's a complete example that uses multiple modules to create a simple clock generator:

```c
#include "pico/stdlib.h"
#include "hardware_init.h"
#include "button_handler.h"
#include "clock_generator.h"
#include "status_display.h"
#include "config.h"

int main() {
    // Initialize hardware and modules
    init_all_hardware();
    button_handler_init();
    clock_generator_init();
    status_display_init();
    
    printf("Simple Clock Generator Starting...\n");
    
    while (true) {
        // Handle button presses for mode switching
        if (button_pressed(BUTTON_SINGLE_STEP, 0)) {
            // Toggle manual clock
            toggle_clock_output();
            printf("Clock toggled\n");
        }
        
        if (button_pressed(BUTTON_LOW_FREQ, 1)) {
            // Start variable frequency mode
            printf("Starting variable frequency mode\n");
            while (!button_pressed(BUTTON_HIGH_FREQ, 2)) {
                update_low_frequency();
                print_status();
                sleep_ms(100);
            }
            stop_all_clock_generation();
        }
        
        if (button_pressed(BUTTON_HIGH_FREQ, 2)) {
            // Start high frequency mode
            printf("Starting 1MHz output\n");
            start_high_frequency();
            sleep_ms(5000);
            stop_high_frequency();
            printf("Stopped 1MHz output\n");
        }
        
        update_leds();
        sleep_ms(10);
    }
    
    return 0;
}
```

## CMakeLists.txt Configuration

Add the modules to your `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
project(my_clock_project)
pico_sdk_init()

add_executable(my_clock_project
    main.c
    hardware_init.c
    button_handler.c
    clock_generator.c
    uart_control.c
    reset_control.c
    power_control.c
    status_display.c
    # Add header files for IDE support
    config.h
    hardware_init.h
    button_handler.h
    clock_generator.h
    uart_control.h
    reset_control.h
    power_control.h
    status_display.h
)

target_link_libraries(my_clock_project 
    pico_stdlib
    hardware_gpio
    hardware_adc
    hardware_uart
    hardware_timer
    hardware_pwm
)

pico_add_extra_outputs(my_clock_project)
pico_enable_stdio_usb(my_clock_project 1)
pico_enable_stdio_uart(my_clock_project 1)
```

## Configuration

Create your own `config.h` with pin definitions:

```c
#ifndef CONFIG_H
#define CONFIG_H

// Pin Configuration - Customize for your hardware
#define BUTTON_SINGLE_STEP  2
#define BUTTON_LOW_FREQ     3
#define BUTTON_HIGH_FREQ    4
#define LED_CLOCK_ACTIVITY  5
#define CLOCK_OUTPUT        9
#define POTENTIOMETER_PIN   26

// Add other pin definitions as needed...

// Timing Configuration
#define DEBOUNCE_DELAY_MS   50
#define UPDATE_INTERVAL_MS  10

// Frequency Configuration
#define HIGH_FREQ_OUTPUT    1000000  // 1MHz

#endif // CONFIG_H
```

## Tips and Best Practices

### 1. Module Selection
- Only include modules you actually need
- Each module can work independently
- Some modules work better together (e.g., button_handler + status_display)

### 2. Error Handling
- Check return values from initialization functions
- Handle timer creation failures gracefully
- Validate frequency ranges in your application

### 3. Resource Management
- Call module init functions before using module features
- Stop timers and PWM before changing modes
- Use the stop functions to clean up resources

### 4. Pin Configuration
- Always update `config.h` for your hardware
- Ensure pin assignments don't conflict
- Consider GPIO capabilities (ADC, PWM, etc.)

### 5. Performance
- Use appropriate update intervals
- Don't call expensive functions too frequently
- Consider using interrupts for time-critical operations

## Troubleshooting

### Common Issues

1. **Compilation Errors**
   - Ensure all required files are included
   - Check that `config.h` defines all required constants
   - Verify CMakeLists.txt includes all source files

2. **Hardware Not Responding**
   - Check pin assignments in `config.h`
   - Verify hardware connections
   - Call appropriate init functions

3. **Timing Issues**
   - Adjust debounce delays if needed
   - Check that timers are properly started/stopped
   - Verify system clock configuration

4. **PWM Not Working**
   - Ensure GPIO supports PWM function
   - Check that PWM is properly initialized
   - Verify frequency calculations are correct

For more detailed information about the modules and their implementation, see [REFACTORING.md](REFACTORING.md).