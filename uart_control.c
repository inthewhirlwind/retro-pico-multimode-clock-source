/**
 * UART Control Module for Multimode Clock Source
 */

#include "uart_control.h"
#include "config.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// UART control state variables
static bool uart_clock_running = false;
static uint32_t uart_set_frequency = 0;
static char uart_cmd_buffer[UART_CMD_BUFFER_SIZE];
static uint8_t uart_cmd_index = 0;
static uint32_t uart_menu_timeout = 0;
static bool uart_pwm_active = false;

// Hardware timer variables (legacy - kept for compatibility)
static alarm_id_t uart_alarm_id = 0;
static bool uart_timer_active = false;

// External function declarations
extern void set_mode(clock_mode_t mode);
extern clock_mode_t get_previous_mode(void);
extern void print_status(void);
extern void start_reset_pulse(void);
extern bool get_reset_active(void);
extern void set_power_state(bool state);
extern bool get_power_state(void);
extern void toggle_clock_output(void);
extern bool get_clock_state(void);
extern void set_clock_output(bool state);
extern bool any_button_pressed(void);

void uart_control_init(void) {
    uart_clock_running = false;
    uart_set_frequency = 0;
    uart_cmd_index = 0;
    uart_menu_timeout = 0;
    uart_pwm_active = false;
    uart_timer_active = false;
    uart_alarm_id = 0;
}

void handle_uart_control(void) {
    // Check for button press to exit UART mode
    if (any_button_pressed()) {
        clock_mode_t prev_mode = get_previous_mode();
        printf("Button pressed - returning to %s mode\n", 
               prev_mode == MODE_SINGLE_STEP ? "Single Step" :
               prev_mode == MODE_LOW_FREQ ? "Low Frequency" : "High Frequency");
        set_mode(prev_mode);
        return;
    }
    
    // Check for timeout
    if (to_ms_since_boot(get_absolute_time()) > uart_menu_timeout) {
        clock_mode_t prev_mode = get_previous_mode();
        printf("UART menu timeout - returning to %s mode\n",
               prev_mode == MODE_SINGLE_STEP ? "Single Step" :
               prev_mode == MODE_LOW_FREQ ? "Low Frequency" : "High Frequency");
        set_mode(prev_mode);
        return;
    }
    
    // Check for UART input
    while (uart_is_readable(uart0)) {
        char c = uart_getc(uart0);
        
        // Reset timeout on any input
        uart_menu_timeout = to_ms_since_boot(get_absolute_time()) + UART_MENU_TIMEOUT_MS;
        
        if (c == '\r' || c == '\n') {
            if (uart_cmd_index > 0) {
                uart_cmd_buffer[uart_cmd_index] = '\0';
                printf("\n"); // New line after command
                process_uart_command(uart_cmd_buffer);
                uart_cmd_index = 0;
            } else {
                printf("Cmd> "); // Show prompt for empty commands
            }
        } else if (c == '\b' || c == 127) { // Backspace or DEL
            if (uart_cmd_index > 0) {
                uart_cmd_index--;
                printf("\b \b"); // Erase character from terminal
            }
        } else if (uart_cmd_index < UART_CMD_BUFFER_SIZE - 1 && c >= 32 && c < 127) {
            // Printable ASCII characters only
            uart_cmd_buffer[uart_cmd_index++] = c;
            printf("%c", c); // Echo character
        }
        // Ignore other control characters
    }
}

void show_uart_menu(void) {
    printf("\n=== UART Control Mode ===\n");
    printf("Commands:\n");
    printf("  stop      - Stop the clock\n");
    printf("  toggle    - Toggle clock state once\n");
    printf("  freq <Hz> - Set frequency (1Hz to 1MHz) and run\n");
    printf("  reset     - Trigger reset pulse (6 clock cycles)\n");
    printf("  power on  - Turn power ON\n");
    printf("  power off - Turn power OFF\n");
    printf("  menu      - Show this menu again\n");
    printf("  status    - Show current status\n");
    printf("\nPress any button to return to previous mode\n");
    printf("Mode will timeout after 30 seconds of inactivity\n");
    printf("\nCmd> ");
}

void process_uart_command(const char* cmd) {
    // Trim leading/trailing whitespace and convert to lowercase for comparison
    while (*cmd == ' ') cmd++; // Skip leading spaces
    
    if (strcmp(cmd, "stop") == 0) {
        stop_uart_frequency();
        uart_clock_running = false;
        set_clock_output(false);
        printf("Clock stopped\n");
        
    } else if (strcmp(cmd, "toggle") == 0) {
        stop_uart_frequency(); // Stop any running PWM or timer
        toggle_clock_output();
        printf("Clock toggled to %s\n", get_clock_state() ? "HIGH" : "LOW");
        uart_clock_running = false; // Stop any running frequency
        
    } else if (strncmp(cmd, "freq ", 5) == 0) {
        const char* freq_str = cmd + 5;
        // Skip any spaces after "freq"
        while (*freq_str == ' ') freq_str++;
        
        if (strlen(freq_str) == 0) {
            printf("Missing frequency value. Usage: freq <Hz>\n");
            return;
        }
        
        char* endptr;
        long freq_long = strtol(freq_str, &endptr, 10);
        
        // Check if conversion was successful and value is within range
        if (endptr == freq_str || *endptr != '\0') {
            printf("Invalid frequency format. Use numbers only.\n");
        } else if (freq_long < MIN_UART_FREQ || freq_long > MAX_UART_FREQ) {
            printf("Invalid frequency. Range: %d Hz to %d Hz\n", MIN_UART_FREQ, MAX_UART_FREQ);
        } else {
            uint32_t freq = (uint32_t)freq_long;
            uart_set_frequency = freq;
            start_uart_frequency(freq);
            uart_clock_running = true;
            printf("Frequency set to %lu Hz and running\n", freq);
        }
        
    } else if (strcmp(cmd, "menu") == 0) {
        show_uart_menu();
        
    } else if (strcmp(cmd, "status") == 0) {
        print_status();
        
    } else if (strcmp(cmd, "reset") == 0) {
        if (!get_reset_active()) {
            start_reset_pulse();
            printf("Reset pulse initiated via UART\n");
        } else {
            printf("Reset pulse already active\n");
        }
        
    } else if (strcmp(cmd, "power on") == 0) {
        bool old_power_state = get_power_state();
        set_power_state(true);
        printf("Power turned ON\n");
        
        // If power just turned ON (OFF->ON transition), switch to Mode 1
        if (!old_power_state && get_power_state()) {
            set_mode(MODE_SINGLE_STEP);
            printf("Automatically switched to Mode 1 (Single Step)\n");
        }
        
    } else if (strcmp(cmd, "power off") == 0) {
        set_power_state(false);
        printf("Power turned OFF\n");
        
    } else if (strlen(cmd) == 0) {
        // Empty command, do nothing
        
    } else {
        printf("Unknown command: %s\n", cmd);
        printf("Type 'menu' for help\n");
    }
    
    printf("Cmd> ");
}

void start_uart_frequency(uint32_t frequency) {
    stop_uart_frequency(); // Stop any existing timer or PWM
    
    if (frequency > 0 && frequency <= MAX_UART_FREQ) {
        start_uart_pwm(frequency);
    }
}

void stop_uart_frequency(void) {
    // Stop hardware timer if active
    if (uart_timer_active && uart_alarm_id > 0) {
        cancel_alarm(uart_alarm_id);
        uart_timer_active = false;
        uart_alarm_id = 0;
    }
    // Stop PWM if active
    stop_uart_pwm();
}

void start_uart_pwm(uint32_t frequency) {
    stop_uart_pwm(); // Stop any existing PWM
    
    if (frequency > 0 && frequency <= MAX_UART_FREQ) {
        // Set GPIO function to PWM
        gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_PWM);
        
        // Get PWM slice for this GPIO
        uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
        
        // Calculate PWM parameters for the desired frequency
        // System clock is typically 125MHz
        // We want: PWM_freq = sys_clock / (divider * (wrap + 1))
        // For 50% duty cycle, we set level = wrap / 2
        
        float sys_clock = 125000000.0f; // 125MHz system clock
        float target_freq = (float)frequency;
        
        // Start with a reasonable wrap value to get good resolution
        uint16_t wrap = 1000; // This gives us good resolution for duty cycle
        float divider = sys_clock / (target_freq * (wrap + 1));
        
        // If divider is too high, reduce wrap and recalculate
        if (divider > 255.0f) {
            wrap = (uint16_t)(sys_clock / (target_freq * 255.0f)) - 1;
            if (wrap < 1) wrap = 1;
            divider = sys_clock / (target_freq * (wrap + 1));
        }
        
        // If divider is too low (less than 1), increase wrap and recalculate
        if (divider < 1.0f) {
            wrap = (uint16_t)(sys_clock / target_freq) - 1;
            if (wrap > 65535) wrap = 65535; // Max 16-bit value
            divider = 1.0f; // Minimum divider
        }
        
        // Ensure minimum wrap for 50% duty cycle
        if (wrap < 2) wrap = 2;
        
        // Set PWM configuration
        pwm_set_clkdiv(slice_num, divider);
        pwm_set_wrap(slice_num, wrap);
        
        // Set 50% duty cycle
        uint16_t duty_level = wrap / 2;
        uint channel = pwm_gpio_to_channel(CLOCK_OUTPUT);
        pwm_set_chan_level(slice_num, channel, duty_level);
        
        // Enable PWM
        pwm_set_enabled(slice_num, true);
        uart_pwm_active = true;
        
        // Set clock activity LED on
        gpio_put(LED_CLOCK_ACTIVITY, 1);
    }
}

void stop_uart_pwm(void) {
    if (uart_pwm_active) {
        uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
        pwm_set_enabled(slice_num, false);
        
        // Reset GPIO function back to SIO (software controlled)
        gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_SIO);
        gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
        gpio_put(CLOCK_OUTPUT, 0);
        
        uart_pwm_active = false;
        gpio_put(LED_CLOCK_ACTIVITY, 0);
    }
}

bool get_uart_clock_running(void) {
    return uart_clock_running;
}

uint32_t get_uart_set_frequency(void) {
    return uart_set_frequency;
}

bool get_uart_pwm_active(void) {
    return uart_pwm_active;
}

void set_uart_menu_timeout(uint32_t timeout_ms) {
    uart_menu_timeout = to_ms_since_boot(get_absolute_time()) + timeout_ms;
}

void reset_uart_control_state(void) {
    uart_clock_running = false;
    uart_set_frequency = 0;
    uart_cmd_index = 0;
    stop_uart_frequency();
}