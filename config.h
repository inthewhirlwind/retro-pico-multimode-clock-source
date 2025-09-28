/**
 * Configuration file for Multimode Clock Source
 * 
 * Modify these values to customize the behavior for your specific requirements
 */

#ifndef CONFIG_H
#define CONFIG_H

// Pin Configuration
#define BUTTON_SINGLE_STEP  2   // Button 1: Single step mode
#define BUTTON_LOW_FREQ     3   // Button 2: Low frequency mode  
#define BUTTON_HIGH_FREQ    4   // Button 3: High frequency mode

#define LED_CLOCK_ACTIVITY  5   // Clock activity indicator
#define LED_SINGLE_STEP     6   // Single step mode indicator
#define LED_LOW_FREQ        7   // Low frequency mode indicator  
#define LED_HIGH_FREQ       8   // High frequency mode indicator
#define LED_UART_MODE       10  // UART control mode indicator

#define CLOCK_OUTPUT        9   // Main clock output pin
#define POTENTIOMETER_PIN   26  // ADC0 - Potentiometer input (GPIO 26)

// Timing Configuration
#define DEBOUNCE_DELAY_MS   50      // Button debounce delay in milliseconds
#define UPDATE_INTERVAL_MS  10      // Main loop update interval

// Frequency Configuration
#define MIN_LOW_FREQ        1       // Minimum frequency in Hz for low freq mode
#define MAX_LOW_FREQ_RANGE1 100     // Maximum frequency for first 20% of pot range
#define MAX_LOW_FREQ_RANGE2 100000  // Maximum frequency for remaining 80% of pot range
#define HIGH_FREQ_OUTPUT    1000000 // Fixed high frequency output (1MHz)

// Potentiometer Range Configuration
#define POT_RANGE1_PERCENT  0.2f    // First range covers 20% of pot rotation
#define POT_RANGE2_PERCENT  0.8f    // Second range covers remaining 80%

// PWM Configuration for High Frequency Mode
#define PWM_CLOCK_DIVIDER   125.0f  // Clock divider for 1MHz output
#define PWM_WRAP_VALUE      1       // PWM wrap value
#define PWM_DUTY_CYCLE      1       // 50% duty cycle (1 out of 2)

// PWM Configuration for UART Control Mode
#define UART_PWM_DUTY_CYCLE_PERCENT 50  // 50% duty cycle for UART PWM mode

// UART Configuration
#define UART_BAUD_RATE      115200  // UART baud rate for status output

// UART Control Mode Configuration
#define UART_MENU_TIMEOUT_MS    30000   // Menu timeout in milliseconds (30 seconds)
#define UART_CMD_BUFFER_SIZE    32      // Command buffer size
#define MIN_UART_FREQ           1       // Minimum frequency for UART mode (1Hz)
#define MAX_UART_FREQ           1000000 // Maximum frequency for UART mode (1MHz)

// Second UART Configuration
#define UART1_TX_PIN        16      // UART1 TX pin (GPIO 16)
#define UART1_RX_PIN        17      // UART1 RX pin (GPIO 17)
#define UART1_BAUD_RATE     115200  // Second UART baud rate

#endif // CONFIG_H