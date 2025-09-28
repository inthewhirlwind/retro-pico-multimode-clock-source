cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(multimode_clock_source)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(multimode_clock_source
        main.c
        )

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(multimode_clock_source 
        pico_stdlib
        hardware_gpio
        hardware_adc
        hardware_uart
        hardware_timer
        hardware_pwm
        )

# create map/bin/hex file etc.
pico_add_extra_outputs(multimode_clock_source)

# enable usb output, disable uart output
pico_enable_stdio_usb(multimode_clock_source 1)
pico_enable_stdio_uart(multimode_clock_source 1)