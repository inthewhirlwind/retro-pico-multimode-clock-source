# Usage Examples

## Example 1: Single Step CPU Clock

Use the Single Step Mode to manually clock a retro CPU:

1. Connect Clock Output (GPIO 9) to CPU clock input  
2. Power on - device starts in Single Step Mode (LED 6 lit)
3. Press Button 1 to generate single clock pulses
4. Clock Activity LED (GPIO 5) shows each pulse
5. Perfect for debugging CPU instruction execution

**Expected UART Output:**
```
=== Clock Source Status ===
Mode: Single Step
Status: Waiting for button press
Clock State: LOW
===========================
```

## Example 2: Variable Frequency Clock for Timer Circuits

Use Low-Frequency Mode for adjustable timing:

1. Press Button 2 to enter Low-Frequency Mode (LED 7 lit)
2. Rotate potentiometer to adjust frequency:
   - First 20% rotation: 1Hz to 100Hz (linear)
   - Remaining 80%: 100Hz to 100kHz
3. Clock automatically starts at selected frequency
4. UART shows real-time frequency updates

**Expected UART Output:**
```
=== Clock Source Status ===
Mode: Low Frequency  
Frequency: 2543 Hz
Clock State: HIGH
===========================
```

## Example 3: High-Speed System Clock

Use High-Frequency Mode for fast systems:

1. Press Button 3 to enter High-Frequency Mode (LED 8 lit)
2. 1MHz square wave automatically starts
3. Clock Activity LED stays on (too fast to see individual pulses)
4. Perfect for high-speed retro computer systems

**Expected UART Output:**
```
=== Clock Source Status ===
Mode: High Frequency
Frequency: 1000000 Hz (1MHz)
Clock State: HIGH
===========================
```

## Example 4: Mode Switching During Operation

Switch between modes at any time:

1. Start in Single Step Mode
2. Press Button 2 → switches to Low-Frequency Mode
3. Press Button 3 → switches to High-Frequency Mode  
4. Press Button 1 → switches back to Single Step Mode
5. Each mode change updates LEDs and UART output

## Potentiometer Calibration Example

To verify potentiometer frequency mapping:

1. Enter Low-Frequency Mode (Button 2)
2. Turn pot fully counter-clockwise → should read ~1Hz
3. Turn pot to 20% position → should read ~100Hz  
4. Turn pot fully clockwise → should read ~100kHz
5. Verify smooth transitions across the range

## Interfacing Examples

### 3.3V Logic Interface
```
Pico GPIO 9 ────→ Target Clock Input
Common GND  ────→ Shared ground
```

### 5V Logic Interface  
```
Pico GPIO 9 ────→ Logic Level Converter ────→ 5V Target
3.3V        ────→ LV side power
5V          ────→ HV side power  
GND         ────→ Common ground
```

### Oscilloscope Connection
```
Pico GPIO 9 ────→ Scope Channel 1 (DC coupled)
GND         ────→ Scope ground
```

## Troubleshooting Examples

### Problem: No response to Button 1 in Single Step Mode
**Check:**
- LED 6 is lit (confirming Single Step Mode)
- Button connection to GPIO 2 and GND
- UART output shows "Waiting for button press"

### Problem: Frequency doesn't change with potentiometer
**Check:**  
- LED 7 is lit (confirming Low-Frequency Mode)
- Potentiometer wiring to GPIO 26, 3.3V, and GND
- UART output updates when rotating pot

### Problem: No 1MHz output in High-Frequency Mode
**Check:**
- LED 8 is lit (confirming High-Frequency Mode)  
- Clock Activity LED is on
- Use oscilloscope to verify 1MHz square wave on GPIO 9