# Detailed Wiring Guide

This document provides comprehensive wiring instructions for the Multimode Clock Source project.

## Components List

### Required Components
- 1x Raspberry Pi Pico
- 3x Push buttons (momentary, normally open)
- 4x LEDs (any color, 3mm or 5mm)
- 4x 220Ω resistors (for LED current limiting)
- 1x 10kΩ potentiometer (linear taper)
- 1x Breadboard (half-size or larger)
- Jumper wires (male-to-male)

### Optional Components
- 3x 10kΩ resistors (external pull-ups for buttons - not needed as internal pull-ups are used)
- 1x Logic level converter (if interfacing with 5V systems)

## Pin Assignment Summary

| Function | GPIO Pin | Physical Pin | Connection |
|----------|----------|--------------|------------|
| Button 1 (Single Step) | GPIO 2 | Pin 4 | Button to GND |
| Button 2 (Low Freq) | GPIO 3 | Pin 5 | Button to GND |
| Button 3 (High Freq) | GPIO 4 | Pin 6 | Button to GND |
| Clock Activity LED | GPIO 5 | Pin 7 | LED + 220Ω resistor to GND |
| Single Step LED | GPIO 6 | Pin 9 | LED + 220Ω resistor to GND |
| Low Freq LED | GPIO 7 | Pin 10 | LED + 220Ω resistor to GND |
| High Freq LED | GPIO 8 | Pin 11 | LED + 220Ω resistor to GND |
| Clock Output | GPIO 9 | Pin 12 | To target circuit |
| Potentiometer | GPIO 26 (ADC0) | Pin 31 | Center pin |
| 3.3V Power | 3V3(OUT) | Pin 36 | To potentiometer and breadboard power rail |
| Ground | GND | Pin 3, 8, 13, 18, 23, 28, 33, 38 | To breadboard ground rail |

## Step-by-Step Wiring Instructions

### Step 1: Power Rails
1. Connect Pin 36 (3V3 OUT) to the positive power rail on your breadboard
2. Connect any GND pin (Pin 3, 8, 13, etc.) to the negative power rail on your breadboard

### Step 2: Push Buttons
For each of the three buttons:

**Button 1 (Single Step Mode):**
1. Place button on breadboard
2. Connect one terminal to GPIO 2 (Pin 4) with a jumper wire
3. Connect the other terminal to the ground rail

**Button 2 (Low Frequency Mode):**
1. Place button on breadboard
2. Connect one terminal to GPIO 3 (Pin 5) with a jumper wire
3. Connect the other terminal to the ground rail

**Button 3 (High Frequency Mode):**
1. Place button on breadboard
2. Connect one terminal to GPIO 4 (Pin 6) with a jumper wire
3. Connect the other terminal to the ground rail

### Step 3: LED Indicators
For each LED, connect as follows:

**Clock Activity LED:**
1. Connect LED anode (long leg) to one end of a 220Ω resistor
2. Connect the other end of the resistor to GPIO 5 (Pin 7)
3. Connect LED cathode (short leg) to the ground rail

**Single Step Mode LED:**
1. Connect LED anode (long leg) to one end of a 220Ω resistor
2. Connect the other end of the resistor to GPIO 6 (Pin 9)
3. Connect LED cathode (short leg) to the ground rail

**Low Frequency Mode LED:**
1. Connect LED anode (long leg) to one end of a 220Ω resistor
2. Connect the other end of the resistor to GPIO 7 (Pin 10)
3. Connect LED cathode (short leg) to the ground rail

**High Frequency Mode LED:**
1. Connect LED anode (long leg) to one end of a 220Ω resistor
2. Connect the other end of the resistor to GPIO 8 (Pin 11)
3. Connect LED cathode (short leg) to the ground rail

### Step 4: Potentiometer
1. Connect the center pin (wiper) of the potentiometer to GPIO 26 (Pin 31)
2. Connect one outer pin to the positive power rail (3.3V)
3. Connect the other outer pin to the ground rail

### Step 5: Clock Output
1. Connect GPIO 9 (Pin 12) to your target circuit's clock input
2. If interfacing with 5V logic, use a logic level converter

## Breadboard Layout Example

```
    Power Rails              Pico Connections
    
    3.3V ────────────────────── Pin 36 (3V3 OUT)
     │
     │   ┌──────────────────────────────────┐
     │   │              PICO                │
     │   │                                  │
     └───┤ 36                            3  ├─── GND Rail
         │                                  │      │
    Pot  │ 31 (GPIO 26)                  4  ├──────┼── Button 1
     ├───┤                                  │      │
    3.3V │                               5  ├──────┼── Button 2
     │   │                                  │      │
    GND──┤                               6  ├──────┼── Button 3
         │                                  │      │
         │  7 (GPIO 5)                      │      │
         ├──── LED1 + Resistor ─────────────┼──────┘
         │                                  │
         │  9 (GPIO 6)                      │
         ├──── LED2 + Resistor ─────────────┤
         │                                  │
         │ 10 (GPIO 7)                      │
         ├──── LED3 + Resistor ─────────────┤
         │                                  │
         │ 11 (GPIO 8)                      │
         ├──── LED4 + Resistor ─────────────┤
         │                                  │
         │ 12 (GPIO 9) ──── Clock Output    │
         │                                  │
         └──────────────────────────────────┘
```

## Testing Your Wiring

### Pre-Power Checks
1. **Continuity Test**: Use a multimeter to verify connections
2. **Short Circuit Check**: Ensure no unintended connections between power and ground
3. **LED Polarity**: Verify LED anode/cathode orientation

### Power-On Test Sequence
1. **Connect Pico via USB** (without target circuit connected)
2. **Check Power LEDs**: Some LEDs may be on by default
3. **Test Buttons**: Press each button and observe LED changes
4. **Check UART Output**: Connect to USB serial and verify status messages
5. **Test Potentiometer**: Rotate and observe frequency changes in Low Freq mode

### Troubleshooting Common Issues

**No Response from Buttons:**
- Check button connections to GPIO pins and ground
- Verify buttons are normally open (NO) type
- Check for loose connections

**LEDs Not Working:**
- Verify LED polarity (anode to resistor, cathode to ground)
- Check resistor values (220Ω recommended)
- Test LED with external power source

**No Clock Output:**
- Use oscilloscope or logic analyzer to verify signal
- Check connection to GPIO 9
- Verify mode selection

**Erratic Potentiometer Behavior:**
- Clean potentiometer contacts
- Check wiring to ADC0 (GPIO 26)
- Verify power connections to pot

## Interface with Target Circuits

### 3.3V Logic Systems
- Direct connection to GPIO 9 clock output
- Ensure target input impedance is compatible

### 5V Logic Systems
- Use bidirectional logic level converter
- Connect Pico side to 3.3V rail
- Connect target side to 5V rail
- Route clock signal through converter

### High-Speed Considerations
- Keep clock output traces short
- Use ground plane for signal integrity
- Consider impedance matching for high frequencies
- Add termination resistors if needed

## Final Verification

After completing the wiring:

1. **Visual Inspection**: Check all connections match the pin assignments
2. **Power Test**: Measure 3.3V on power rail with multimeter
3. **Functional Test**: Test each mode and verify proper operation
4. **Signal Quality**: Use oscilloscope to verify clean clock signals
5. **Documentation**: Take photos of your setup for future reference

## Safety Notes

- Never exceed 3.3V on any GPIO pin
- Use appropriate current-limiting resistors for LEDs
- Disconnect power when making wiring changes
- Verify connections before applying power
- Use ESD precautions when handling the Pico