/**
 * Multimode Clock Source for Raspberry Pi Pico
 * 
 * Features:
 * - Single Step Mode: Manual clock toggle with button
 * - Low-Frequency Mode: 1Hz-100Hz (20% pot) and 100Hz-100kHz (80% pot)  
 * - High-Frequency Mode: Fixed 1MHz output
 * - UART Control Mode: UART-controlled frequency from 10Hz to 1MHz
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
#include "config.h"

// Clock modes
typedef enum {
    MODE_SINGLE_STEP,
    MODE_LOW_FREQ,
    MODE_HIGH_FREQ,
    MODE_UART_CONTROL
} clock_mode_t;

// Global variables
static clock_mode_t current_mode = MODE_SINGLE_STEP;
static clock_mode_t previous_mode = MODE_SINGLE_STEP;
static bool clock_state = false;
static uint32_t current_frequency = 0;
static bool single_step_active = false;

// Button debouncing
static uint32_t last_button_time[5] = {0, 0, 0, 0, 0}; // Added reset and power buttons

// Power control variables
static bool power_state = false; // false = OFF (default), true = ON

// Timer for low frequency mode and UART mode
static struct repeating_timer low_freq_timer;
static bool timer_active = false;

// Hardware timer for UART mode (legacy - kept for compatibility during transition)
static alarm_id_t uart_alarm_id = 0;
static bool uart_timer_active = false;

// UART control mode variables
static bool uart_clock_running = false;
static uint32_t uart_set_frequency = 0;
static char uart_cmd_buffer[UART_CMD_BUFFER_SIZE];
static uint8_t uart_cmd_index = 0;
static uint32_t uart_menu_timeout = 0;
static bool uart_pwm_active = false;

// Reset functionality variables
static bool reset_active = false;
static bool reset_output_state = true; // Reset output is normally high
static uint32_t reset_cycle_count = 0;
static uint32_t reset_start_time = 0;
static uint32_t reset_high_led_timer = 0;
static bool reset_waiting_for_edge = false; // For Mode 1 edge detection
static bool last_clock_state_for_reset = false;

// Function prototypes
void init_gpio(void);
void init_adc(void);
void init_uart(void);
void init_second_uart(void);
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
void print_status_to_uart1(void);
uint32_t calculate_frequency_from_pot(uint16_t adc_value);
void handle_uart_control(void);
void show_uart_menu(void);
void process_uart_command(const char* cmd);
void start_uart_frequency(uint32_t frequency);
void stop_uart_frequency(void);
void start_uart_pwm(uint32_t frequency);
void stop_uart_pwm(void);
bool any_button_pressed(void);

// Reset functionality prototypes
void handle_reset_button(void);
void start_reset_pulse(void);
void update_reset_state(void);
void update_reset_leds(void);
void set_reset_output(bool state);

// Power control prototypes
void handle_power_button(void);
void toggle_power_state(void);
void set_power_state(bool state);
void update_power_led(void);

int main() {
    stdio_init_all();
    
    // Initialize hardware
    init_gpio();
    init_adc();
    init_uart();
    init_second_uart();
    
    // Set initial mode
    set_mode(MODE_SINGLE_STEP);
    
    printf("Multimode Clock Source Starting...\n");
    uart_puts(uart1, "Multimode Clock Source Starting...\n");
    printf("Press and hold any button for 3 seconds to enter UART Control Mode\n");
    print_status();
    
    uint32_t button_hold_start = 0;
    bool button_held = false;
    
    while (true) {
        // Check for button hold to enter UART mode (only if not in UART mode)
        if (current_mode != MODE_UART_CONTROL) {
            if (any_button_pressed()) {
                if (!button_held) {
                    button_hold_start = to_ms_since_boot(get_absolute_time());
                    button_held = true;
                    printf("Hold button for UART mode...\n");
                } else {
                    uint32_t hold_time = to_ms_since_boot(get_absolute_time()) - button_hold_start;
                    if (hold_time >= 3000) { // 3 seconds
                        previous_mode = current_mode;
                        set_mode(MODE_UART_CONTROL);
                        printf("Entered UART Control Mode\n");
                        button_held = false;
                    }
                }
            } else {
                if (button_held) {
                    // Button released before 3 seconds - handle normal button press
                    // Use a small delay to ensure button state is stable
                    sleep_ms(50); // Debounce delay
                    handle_buttons();
                }
                button_held = false;
            }
        }
        
        // Handle mode-specific updates
        if (current_mode == MODE_LOW_FREQ) {
            update_low_frequency();
        } else if (current_mode == MODE_UART_CONTROL) {
            handle_uart_control();
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
    
    gpio_init(BUTTON_RESET);
    gpio_set_dir(BUTTON_RESET, GPIO_IN);
    gpio_pull_up(BUTTON_RESET);
    
    gpio_init(BUTTON_POWER);
    gpio_set_dir(BUTTON_POWER, GPIO_IN);
    gpio_pull_up(BUTTON_POWER);
    
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
    
    gpio_init(LED_UART_MODE);
    gpio_set_dir(LED_UART_MODE, GPIO_OUT);
    gpio_put(LED_UART_MODE, 0);
    
    gpio_init(LED_RESET_LOW);
    gpio_set_dir(LED_RESET_LOW, GPIO_OUT);
    gpio_put(LED_RESET_LOW, 0);
    
    gpio_init(LED_RESET_HIGH);
    gpio_set_dir(LED_RESET_HIGH, GPIO_OUT);
    gpio_put(LED_RESET_HIGH, 0);
    
    gpio_init(LED_POWER_ON);
    gpio_set_dir(LED_POWER_ON, GPIO_OUT);
    gpio_put(LED_POWER_ON, 0); // Start with power LED off
    
    // Initialize clock output
    gpio_init(CLOCK_OUTPUT);
    gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
    gpio_put(CLOCK_OUTPUT, 0);
    
    // Initialize reset output (normally high)
    gpio_init(RESET_OUTPUT);
    gpio_set_dir(RESET_OUTPUT, GPIO_OUT);
    gpio_put(RESET_OUTPUT, 1);
    
    // Initialize power control output (HIGH = power OFF, default state)
    gpio_init(POWER_OUTPUT);
    gpio_set_dir(POWER_OUTPUT, GPIO_OUT);
    gpio_put(POWER_OUTPUT, 1); // Start with power OFF
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

void init_second_uart(void) {
    // Initialize UART1 on GPIO pins 16 (TX) and 17 (RX)
    uart_init(uart1, UART1_BAUD_RATE);
    
    // Set the GPIO pin functions to UART
    gpio_set_function(UART1_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART1_RX_PIN, GPIO_FUNC_UART);
    
    // Set UART format (8 data bits, 1 stop bit, no parity)
    uart_set_format(uart1, 8, 1, UART_PARITY_NONE);
    
    // Turn off FIFO - we want to do this byte by byte
    uart_set_fifo_enabled(uart1, false);
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
    if (uart_timer_active) {
        cancel_alarm(uart_alarm_id);
        uart_timer_active = false;
        uart_alarm_id = 0;
    }
    stop_high_frequency();
    stop_uart_frequency(); // This now handles both timer and PWM cleanup
    
    current_mode = mode;
    single_step_active = false;
    uart_clock_running = false;
    set_clock_output(false);
    
    switch (mode) {
        case MODE_SINGLE_STEP:
            current_frequency = 0;
            break;
            
        case MODE_LOW_FREQ:
            update_low_frequency();
            break;
            
        case MODE_HIGH_FREQ:
            current_frequency = HIGH_FREQ_OUTPUT; // 1MHz
            start_high_frequency();
            break;
            
        case MODE_UART_CONTROL:
            current_frequency = 0;
            uart_set_frequency = 0;
            uart_menu_timeout = to_ms_since_boot(get_absolute_time()) + UART_MENU_TIMEOUT_MS;
            show_uart_menu();
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
    gpio_put(LED_UART_MODE, 0);
    
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
        case MODE_UART_CONTROL:
            gpio_put(LED_UART_MODE, 1);
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
    
    if (pot_position <= POT_RANGE1_PERCENT) {
        // First 20%: MIN_LOW_FREQ to MAX_LOW_FREQ_RANGE1 linear
        return (uint32_t)(MIN_LOW_FREQ + (pot_position / POT_RANGE1_PERCENT) * (MAX_LOW_FREQ_RANGE1 - MIN_LOW_FREQ));
    } else {
        // Remaining 80%: MAX_LOW_FREQ_RANGE1 to MAX_LOW_FREQ_RANGE2
        float remaining_position = (pot_position - POT_RANGE1_PERCENT) / POT_RANGE2_PERCENT;
        return (uint32_t)(MAX_LOW_FREQ_RANGE1 + remaining_position * (MAX_LOW_FREQ_RANGE2 - MAX_LOW_FREQ_RANGE1));
    }
}

bool low_freq_timer_callback(struct repeating_timer *t) {
    toggle_clock_output();
    return true; // Continue repeating
}

void start_high_frequency(void) {
    // Set GPIO function to PWM
    gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_PWM);
    
    // Use PWM to generate 1MHz square wave
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    
    // Set PWM frequency to 1MHz with 50% duty cycle
    // System clock is typically 125MHz
    // For 1MHz: divider = 125, wrap = 1 (gives 125MHz / 125 / 1 = 1MHz)
    pwm_set_clkdiv(slice_num, PWM_CLOCK_DIVIDER);
    pwm_set_wrap(slice_num, PWM_WRAP_VALUE);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, PWM_DUTY_CYCLE); // 50% duty cycle
    
    // Enable PWM
    pwm_set_enabled(slice_num, true);
    
    // Set clock activity LED on
    gpio_put(LED_CLOCK_ACTIVITY, 1);
}

void stop_high_frequency(void) {
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    pwm_set_enabled(slice_num, false);
    
    // Reset GPIO function back to SIO (software controlled)
    gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_SIO);
    gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
    gpio_put(CLOCK_OUTPUT, 0);
    
    gpio_put(LED_CLOCK_ACTIVITY, 0);
}

void print_status_to_uart1(void) {
    const char* status_header = "\n=== Clock Source Status ===\n";
    const char* status_footer = "===========================\n\n";
    
    // Send header
    uart_puts(uart1, status_header);
    
    switch (current_mode) {
        case MODE_SINGLE_STEP:
            uart_puts(uart1, "Mode: Single Step\n");
            if (single_step_active) {
                uart_puts(uart1, "Status: Active\n");
            } else {
                uart_puts(uart1, "Status: Waiting for button press\n");
            }
            break;
            
        case MODE_LOW_FREQ:
            uart_puts(uart1, "Mode: Low Frequency\n");
            // Format frequency string
            char freq_str[32];
            snprintf(freq_str, sizeof(freq_str), "Frequency: %lu Hz\n", current_frequency);
            uart_puts(uart1, freq_str);
            break;
            
        case MODE_HIGH_FREQ:
            uart_puts(uart1, "Mode: High Frequency\n");
            char hfreq_str[32];
            snprintf(hfreq_str, sizeof(hfreq_str), "Frequency: %lu Hz (1MHz)\n", current_frequency);
            uart_puts(uart1, hfreq_str);
            break;
            
        case MODE_UART_CONTROL:
            uart_puts(uart1, "Mode: UART Control\n");
            if (uart_clock_running && uart_set_frequency > 0) {
                char ufreq_str[32];
                snprintf(ufreq_str, sizeof(ufreq_str), "Frequency: %lu Hz\n", uart_set_frequency);
                uart_puts(uart1, ufreq_str);
                uart_puts(uart1, "Status: Running\n");
            } else {
                uart_puts(uart1, "Status: Stopped\n");
            }
            break;
    }
    
    // Clock state
    if (current_mode == MODE_UART_CONTROL && uart_pwm_active) {
        uart_puts(uart1, "Clock State: PWM Active\n");
    } else if (current_mode == MODE_HIGH_FREQ) {
        uart_puts(uart1, "Clock State: PWM Active\n");
    } else if (clock_state) {
        uart_puts(uart1, "Clock State: HIGH\n");
    } else {
        uart_puts(uart1, "Clock State: LOW\n");
    }
    
    // Power state
    if (power_state) {
        uart_puts(uart1, "Power State: ON\n");
    } else {
        uart_puts(uart1, "Power State: OFF\n");
    }
    
    // Send footer
    uart_puts(uart1, status_footer);
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
            
        case MODE_UART_CONTROL:
            printf("Mode: UART Control\n");
            if (uart_clock_running && uart_set_frequency > 0) {
                printf("Frequency: %lu Hz\n", uart_set_frequency);
                printf("Status: Running\n");
            } else {
                printf("Status: Stopped\n");
            }
            break;
    }
    
    printf("Clock State: %s\n", 
           (current_mode == MODE_UART_CONTROL && uart_pwm_active) ? "PWM Active" :
           (current_mode == MODE_HIGH_FREQ) ? "PWM Active" :
           (clock_state ? "HIGH" : "LOW"));
    printf("Power State: %s\n", power_state ? "ON" : "OFF");
    printf("===========================\n\n");
    
    // Also send status to second UART
    print_status_to_uart1();
}

void handle_uart_control(void) {
    // Check for button press to exit UART mode
    if (any_button_pressed()) {
        printf("Button pressed - returning to %s mode\n", 
               previous_mode == MODE_SINGLE_STEP ? "Single Step" :
               previous_mode == MODE_LOW_FREQ ? "Low Frequency" : "High Frequency");
        set_mode(previous_mode);
        return;
    }
    
    // Check for timeout
    if (to_ms_since_boot(get_absolute_time()) > uart_menu_timeout) {
        printf("UART menu timeout - returning to %s mode\n",
               previous_mode == MODE_SINGLE_STEP ? "Single Step" :
               previous_mode == MODE_LOW_FREQ ? "Low Frequency" : "High Frequency");
        set_mode(previous_mode);
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
        printf("Clock toggled to %s\n", clock_state ? "HIGH" : "LOW");
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
        if (!reset_active) {
            start_reset_pulse();
            printf("Reset pulse initiated via UART\n");
        } else {
            printf("Reset pulse already active\n");
        }
        
    } else if (strcmp(cmd, "power on") == 0) {
        set_power_state(true);
        printf("Power turned ON\n");
        
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

bool any_button_pressed(void) {
    return !gpio_get(BUTTON_SINGLE_STEP) || 
           !gpio_get(BUTTON_LOW_FREQ) || 
           !gpio_get(BUTTON_HIGH_FREQ);
}

// Reset functionality implementation
void handle_reset_button(void) {
    // Check for reset button press (positive edge triggered, active low with pull-up)
    bool pressed = !gpio_get(BUTTON_RESET);
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (pressed && (current_time - last_button_time[3] > DEBOUNCE_DELAY_MS)) {
        last_button_time[3] = current_time;
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
    reset_waiting_for_edge = (current_mode == MODE_SINGLE_STEP); // Mode 1 needs edge detection
    last_clock_state_for_reset = clock_state;
    set_reset_output(false); // Start reset pulse (low)
    printf("Reset pulse started, mode: %d\n", current_mode + 1);
}

void update_reset_state(void) {
    if (!reset_active) return;
    
    if (current_mode == MODE_SINGLE_STEP) {
        // Mode 1: Track clock low-to-high transitions
        if (reset_waiting_for_edge && !last_clock_state_for_reset && clock_state) {
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
        last_clock_state_for_reset = clock_state;
    } else {
        // Modes 2, 3, 4: Count actual clock cycles using timing
        uint32_t elapsed_ms = to_ms_since_boot(get_absolute_time()) - reset_start_time;
        uint32_t required_ms = 0;
        
        // Calculate required time based on current frequency
        if (current_mode == MODE_LOW_FREQ && current_frequency > 0) {
            // For low frequency mode, use current frequency
            required_ms = (RESET_CYCLES * 1000) / current_frequency;
        } else if (current_mode == MODE_HIGH_FREQ) {
            // For high frequency mode, use fixed 1MHz
            required_ms = (RESET_CYCLES * 1000) / HIGH_FREQ_OUTPUT;
            if (required_ms == 0) required_ms = 1; // Minimum 1ms for visibility
        } else if (current_mode == MODE_UART_CONTROL && uart_set_frequency > 0) {
            // For UART control mode, use set frequency
            required_ms = (RESET_CYCLES * 1000) / uart_set_frequency;
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

// Power control functionality implementation
void handle_power_button(void) {
    // Check for power button press (positive edge triggered, active low with pull-up)
    bool pressed = !gpio_get(BUTTON_POWER);
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    
    if (pressed && (current_time - last_button_time[4] > DEBOUNCE_DELAY_MS)) {
        last_button_time[4] = current_time;
        toggle_power_state();
        printf("Power %s\n", power_state ? "ON" : "OFF");
    }
}

void toggle_power_state(void) {
    set_power_state(!power_state);
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