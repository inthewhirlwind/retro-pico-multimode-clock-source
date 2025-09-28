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
#include "config.h"

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

// Hardware timer state for low frequency mode
static bool low_freq_pwm_active = false;

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
void start_low_frequency(uint32_t frequency);
void stop_low_frequency(void);
void calculate_pwm_params(uint32_t frequency, float *clkdiv, uint16_t *wrap, uint16_t *level);
void start_high_frequency(void);
void stop_high_frequency(void);
void print_status(void);
void print_status_to_uart1(void);
uint32_t calculate_frequency_from_pot(uint16_t adc_value);

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
    print_status();
    
    while (true) {
        handle_buttons();
        
        // Update low frequency if in that mode
        if (current_mode == MODE_LOW_FREQ) {
            update_low_frequency();
        }
        
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
    stop_low_frequency();
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
            current_frequency = HIGH_FREQ_OUTPUT; // 1MHz
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
            // Update clock activity LED for single step mode
            gpio_put(LED_CLOCK_ACTIVITY, clock_state);
            break;
        case MODE_LOW_FREQ:
            gpio_put(LED_LOW_FREQ, 1);
            // Clock activity LED is managed by start_low_frequency/stop_low_frequency
            break;
        case MODE_HIGH_FREQ:
            gpio_put(LED_HIGH_FREQ, 1);
            // Clock activity LED is managed by start_high_frequency/stop_high_frequency
            break;
    }
}

void toggle_clock_output(void) {
    clock_state = !clock_state;
    set_clock_output(clock_state);
    update_leds();
}

void set_clock_output(bool state) {
    clock_state = state;
    // Only set GPIO output if not using PWM (i.e., in single step mode)
    if (current_mode == MODE_SINGLE_STEP) {
        gpio_put(CLOCK_OUTPUT, state);
        gpio_put(LED_CLOCK_ACTIVITY, state);
    }
}

void update_low_frequency(void) {
    uint16_t adc_value = adc_read();
    uint32_t new_frequency = calculate_frequency_from_pot(adc_value);
    
    if (new_frequency != current_frequency) {
        current_frequency = new_frequency;
        
        // Stop current PWM
        stop_low_frequency();
        
        // Start new PWM if frequency > 0
        if (current_frequency > 0) {
            start_low_frequency(current_frequency);
        }
        
        print_status();
    }
}

void calculate_pwm_params(uint32_t frequency, float *clkdiv, uint16_t *wrap, uint16_t *level) {
    // System clock is typically 125MHz
    const uint32_t system_clock_hz = 125000000;
    
    // We need to find clkdiv and wrap such that:
    // output_frequency = system_clock_hz / clkdiv / (wrap + 1)
    // We want 50% duty cycle, so level = wrap / 2
    
    // Hardware limitation: PWM minimum frequency is ~7.5Hz due to 16-bit wrap limit
    // For frequencies below 8Hz, we use the closest achievable frequency
    if (frequency < 8) {
        // Use maximum divider and wrap for lowest possible frequency (~7.5Hz)
        *clkdiv = 255.0f;
        *wrap = 65535; // Maximum 16-bit value
        *level = *wrap / 2;
    } else if (frequency < 1000) {
        // For frequencies 8Hz-1kHz, optimize for accuracy using max divider
        *clkdiv = 255.0f;
        *wrap = (system_clock_hz / (255 * frequency)) - 1;
        
        // Ensure wrap doesn't exceed 16-bit limit
        if (*wrap > 65535) {
            *wrap = 65535;
        }
        
        *level = *wrap / 2;
    } else {
        // For frequencies >= 1kHz, use smaller wrap values for better resolution
        *wrap = 124; // This gives us good resolution
        *clkdiv = (float)system_clock_hz / (frequency * (*wrap + 1));
        
        // Ensure clkdiv is within valid range
        if (*clkdiv < 1.0f) {
            *clkdiv = 1.0f;
            // Recalculate wrap for the minimum clock divider
            *wrap = (system_clock_hz / frequency) - 1;
            if (*wrap > 65535) *wrap = 65535; // PWM wrap is 16-bit
        }
        
        *level = *wrap / 2;
    }
}

void start_low_frequency(uint32_t frequency) {
    // Calculate PWM parameters for the desired frequency
    float clkdiv;
    uint16_t wrap, level;
    calculate_pwm_params(frequency, &clkdiv, &wrap, &level);
    
    // Set GPIO function to PWM
    gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_PWM);
    
    // Configure PWM
    uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_chan_level(slice_num, PWM_CHAN_A, level); // 50% duty cycle
    
    // Enable PWM
    pwm_set_enabled(slice_num, true);
    low_freq_pwm_active = true;
    
    // Set clock activity LED on
    gpio_put(LED_CLOCK_ACTIVITY, 1);
}

void stop_low_frequency(void) {
    if (low_freq_pwm_active) {
        uint slice_num = pwm_gpio_to_slice_num(CLOCK_OUTPUT);
        pwm_set_enabled(slice_num, false);
        
        // Reset GPIO function back to SIO (software controlled)
        gpio_set_function(CLOCK_OUTPUT, GPIO_FUNC_SIO);
        gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
        gpio_put(CLOCK_OUTPUT, 0);
        
        low_freq_pwm_active = false;
        gpio_put(LED_CLOCK_ACTIVITY, 0);
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
    }
    
    // Clock state
    if (clock_state) {
        uart_puts(uart1, "Clock State: HIGH\n");
    } else {
        uart_puts(uart1, "Clock State: LOW\n");
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
    }
    
    printf("Clock State: %s\n", clock_state ? "HIGH" : "LOW");
    printf("===========================\n\n");
    
    // Also send status to second UART
    print_status_to_uart1();
}