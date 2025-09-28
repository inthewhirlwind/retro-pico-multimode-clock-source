# Retro Pico Multimode Clock Source

A versatile multi-mode clock generator for retro computing applications using a Raspberry Pi Pico. This project provides three distinct clock modes with LED indicators and UART status output.

## Features

### Clock Modes

1. **Single Step Mode** (Default)
   - Manual clock toggle using the first push button
   - Debounced positive edge trigger
   - LED indicator shows clock output activity
   - Perfect for single-stepping through CPU cycles

2. **Low-Frequency Mode**
   - Frequency adjustable via potentiometer
   - Two frequency ranges:
     - **1Hz to 100Hz**: First 20% of potentiometer rotation (linear progression)
     - **100Hz to 100kHz**: Remaining 80% of potentiometer rotation
   - Continuous frequency adjustment
   - Real-time UART frequency display

3. **High-Frequency Mode**
   - Fixed 1MHz square wave output
   - Uses PWM for precise timing
   - Ideal for high-speed clock requirements

### User Interface

- **3 Push Buttons**:
  - Button 1: Single Step Mode toggle/selection
  - Button 2: Low-Frequency Mode selection
  - Button 3: High-Frequency Mode selection
- **4 LED Indicators**:
  - Clock activity LED (shows current clock state)
  - Mode indicator LEDs (one for each mode)
- **UART Output**: Real-time mode and frequency information
- **Potentiometer**: Frequency control for Low-Frequency Mode

## Hardware Requirements

- Raspberry Pi Pico (or Pico W)
- 3 Push buttons (momentary, normally open)
- 4 LEDs with appropriate current-limiting resistors (220Ω recommended)
- 1 Potentiometer (10kΩ recommended)
- Breadboard and jumper wires
- 3.3V pull-up resistors for buttons (optional, internal pull-ups used)

## Pin Assignments

| Component | GPIO Pin | Description |
|-----------|----------|-------------|
| Button 1 (Single Step) | GPIO 2 | Single step mode button |
| Button 2 (Low Freq) | GPIO 3 | Low frequency mode button |
| Button 3 (High Freq) | GPIO 4 | High frequency mode button |
| Clock Activity LED | GPIO 5 | Shows clock output state |
| Single Step Mode LED | GPIO 6 | Single step mode indicator |
| Low Freq Mode LED | GPIO 7 | Low frequency mode indicator |
| High Freq Mode LED | GPIO 8 | High frequency mode indicator |
| Clock Output | GPIO 9 | Main clock signal output |
| UART1 TX | GPIO 16 | Second UART transmit (status output) |
| UART1 RX | GPIO 17 | Second UART receive (not used) |
| Potentiometer | GPIO 26 (ADC0) | Frequency control input |

## Breadboard Wiring Diagram

```
Raspberry Pi Pico
                    ┌─────────────────────┐
                    │                     │
GPIO 2 ──────────────┤ 2               40 │──────────── VBUS
GPIO 3 ──────────────┤ 3               39 │──────────── VSYS  
GPIO 4 ──────────────┤ 4               38 │──────────── GND
                    │ 5               37 │──────────── 3V3_EN
GPIO 5 ──────────────┤ 6               36 │──────────── 3V3(OUT)
GPIO 6 ──────────────┤ 7               35 │──────────── ADC_VREF
GPIO 7 ──────────────┤ 8               34 │──────────── GPIO 28
GPIO 8 ──────────────┤ 9               33 │──────────── GND
GPIO 9 ──────────────┤ 10              32 │──────────── GPIO 27
                    │ 11              31 │──────────── GPIO 26 (ADC0) ── Potentiometer
                    │ 12              30 │
                    │ 13              29 │
                    │ 14              28 │
                    │ 15              27 │
GPIO 16 (UART1 TX) ──┤ 16              26 │
GPIO 17 (UART1 RX) ──┤ 17              25 │
                    │                     │
                    └─────────────────────┘

Components:
- Buttons: Connect between GPIO pin and GND (internal pull-up enabled)
- LEDs: Connect via 220Ω resistor between GPIO pin and GND
- Potentiometer: Center pin to GPIO 26, outer pins to 3.3V and GND
- Clock Output: Connect to your target circuit input
- UART1: Connect GPIO 16 (TX) to receive pin of external UART device
```

### Detailed Wiring Instructions

1. **Push Buttons**:
   - Connect one terminal of each button to the respective GPIO pin
   - Connect the other terminal to GND
   - Internal pull-up resistors are enabled in software

2. **LEDs**:
   - Connect cathode (short leg) to GND
   - Connect anode (long leg) to GPIO pin through 220Ω resistor

3. **Potentiometer**:
   - Connect center wiper to GPIO 26 (ADC0)
   - Connect one outer terminal to 3.3V
   - Connect other outer terminal to GND

4. **Clock Output**:
   - Connect GPIO 9 to your target circuit
   - May need level shifting for 5V systems

5. **Second UART Output**:
   - Connect GPIO 16 (UART1 TX) to the receive pin of your external UART device
   - Connect GPIO 17 (UART1 RX) to the transmit pin if bidirectional communication is needed
   - Connect GND to the ground of your external UART device
   - Baud rate: 115200, 8 data bits, 1 stop bit, no parity

## Building and Flashing

### Prerequisites

1. Install the Raspberry Pi Pico SDK
2. Set the `PICO_SDK_PATH` environment variable
3. Install CMake and build tools

### Build Instructions

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build the project
make -j4
```

### Flashing

1. Hold the BOOTSEL button on the Pico while connecting to USB
2. Copy the generated `multimode_clock_source.uf2` file to the RPI-RP2 drive
3. The Pico will automatically reboot and start running the program

## Operation

### Mode Selection

- **Power On**: Device starts in Single Step Mode
- **Button Press**: 
  - Button 1: Activates Single Step Mode (or toggles clock if already in this mode)
  - Button 2: Switches to Low-Frequency Mode
  - Button 3: Switches to High-Frequency Mode

### Single Step Mode
- Press Button 1 to manually toggle the clock output
- Clock activity LED shows current clock state
- Perfect for debugging and single-stepping through circuits

### Low-Frequency Mode
- Adjust potentiometer to change frequency
- Real-time frequency display via UART
- Frequency ranges:
  - 0-20% rotation: 1Hz to 100Hz (linear)
  - 20-100% rotation: 100Hz to 100kHz

### High-Frequency Mode
- Automatically outputs 1MHz square wave
- Uses hardware PWM for precise timing
- Clock activity LED remains on during operation

## UART Output

The device provides status output via two UART interfaces:

1. **USB CDC (Primary)**: Connect to the Pico's USB CDC interface at 115200 baud
2. **Hardware UART1 (Secondary)**: Available on GPIO 16 (TX) and GPIO 17 (RX) at 115200 baud

Both UARTs output the same real-time status information:

```
=== Clock Source Status ===
Mode: Low Frequency
Frequency: 1543 Hz
Clock State: HIGH
===========================
```

The secondary UART allows for external monitoring without requiring a USB connection to a computer.

## Technical Details

### Button Debouncing
- 50ms debounce delay implemented in software
- Uses timestamp comparison for reliable operation

### Frequency Generation
- **Low frequencies (1Hz-100kHz)**: Hardware PWM for precise timing and reduced CPU overhead
- **High frequency (1MHz)**: Hardware PWM for accuracy

### ADC Resolution
- 12-bit ADC provides 4096 discrete frequency steps
- Smooth frequency transitions across the entire range

## Troubleshooting

1. **No clock output**: Check LED indicators to verify mode selection
2. **Erratic frequency**: Ensure potentiometer connections are secure
3. **Buttons not responding**: Verify button connections and check for proper debouncing
4. **No UART output**: 
   - USB CDC: Ensure USB connection and correct baud rate (115200)
   - Hardware UART1: Check GPIO 16 (TX) and GPIO 17 (RX) connections, verify 115200 baud rate
5. **Second UART not working**: Verify GPIO 16/17 are not being used by other functions and that the receiving device is properly connected

## License

This project is open source. Feel free to modify and adapt for your retro computing needs!
