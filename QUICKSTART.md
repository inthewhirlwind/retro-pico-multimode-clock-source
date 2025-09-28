# Quick Start Guide

## Building the Project

### Prerequisites
1. Install Raspberry Pi Pico SDK
2. Set environment variable: `export PICO_SDK_PATH=/path/to/pico-sdk`
3. Install CMake and build tools

### Build Commands
```bash
./build.sh
```

Or manually:
```bash
mkdir build && cd build
cmake ..
make -j4
```

## Flashing

1. Hold BOOTSEL button on Pico while connecting USB
2. Copy `build/multimode_clock_source.uf2` to RPI-RP2 drive
3. Pico will reboot and start the program

## Quick Pin Reference

| Function | GPIO | Physical Pin |
|----------|------|--------------|
| Button 1 | 2 | 4 |
| Button 2 | 3 | 5 |
| Button 3 | 4 | 6 |
| Clock Activity LED | 5 | 7 |
| Mode LEDs | 6,7,8 | 9,10,11 |
| Clock Output | 9 | 12 |
| UART1 TX | 16 | 21 |
| UART1 RX | 17 | 22 |
| Potentiometer | 26 | 31 |

## Operation

### Mode Selection
- **Button 1**: Single Step Mode (manual toggle)
- **Button 2**: Low Frequency Mode (1Hz-100kHz via pot)
- **Button 3**: High Frequency Mode (fixed 1MHz)

### LED Indicators
- **Clock Activity**: Shows current clock state
- **Mode LEDs**: One lit per active mode

### UART Output
Connect USB for real-time status at 115200 baud, or use GPIO 16 (UART1 TX) for external monitoring.

## Troubleshooting

**Build fails**: Check PICO_SDK_PATH environment variable
**No clock output**: Verify wiring and mode selection
**Buttons don't work**: Check connections to ground
**LEDs not working**: Verify polarity and resistor values