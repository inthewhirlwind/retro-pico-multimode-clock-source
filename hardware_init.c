/**
 * Hardware Initialization Module for Multimode Clock Source
 */

#include "hardware_init.h"
#include "config.h"

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
    gpio_put(LED_POWER_ON, 0);
    
    // Initialize outputs
    gpio_init(CLOCK_OUTPUT);
    gpio_set_dir(CLOCK_OUTPUT, GPIO_OUT);
    gpio_put(CLOCK_OUTPUT, 0);
    
    gpio_init(RESET_OUTPUT);
    gpio_set_dir(RESET_OUTPUT, GPIO_OUT);
    gpio_put(RESET_OUTPUT, 1); // Reset output is normally high
    
    gpio_init(POWER_OUTPUT);
    gpio_set_dir(POWER_OUTPUT, GPIO_OUT);
    gpio_put(POWER_OUTPUT, 1); // Power output is inverted: HIGH = power OFF (default)
}

void init_adc(void) {
    adc_init();
    adc_gpio_init(POTENTIOMETER_PIN);
    adc_select_input(0); // ADC0 corresponds to GPIO 26
}

void init_uart(void) {
    // Primary UART is initialized by stdio_init_all()
    // This function is kept for consistency and future expansion
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

void init_all_hardware(void) {
    stdio_init_all();
    init_gpio();
    init_adc();
    init_uart();
    init_second_uart();
}