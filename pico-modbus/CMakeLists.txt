# Set minimum required version of CMake
cmake_minimum_required(VERSION 3.6)

# Set board type because we are building for PicoW
set(PICO_BOARD pico_w)

# Include build functions from Pico SDK
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

# Set name of project (as PROJECT_NAME) and C/C   standards
project(pico-modbus C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Creates a pico-sdk subdirectory in our project for the libraries
pico_sdk_init()




add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        -fexceptions
)
file(GLOB MQTT_SOURCES "paho.mqtt.embedded-c/MQTTPacket/src/*.c")

# Tell CMake where to find the executable source file
add_executable(${PROJECT_NAME}
        main.cpp
        modbus/nanomodbus.h
        modbus/nanomodbus.c
        uart/PicoUart.cpp
        uart/PicoUart.h
        uart/RingBuffer.cpp
        uart/RingBuffer.h
        mqtt/Countdown.cpp
        mqtt/Countdown.h
        mqtt/IPStack.cpp
        mqtt/IPStack.h
        mqtt/lwipopts.h
        modbus/ModbusRegister.cpp
        modbus/ModbusRegister.h
        modbus/ModbusClient.cpp
        modbus/ModbusClient.h
        ${MQTT_SOURCES}
        display/framebuf.cpp
        display/framebuf.h
        display/mono_vlsb.cpp
        display/mono_vlsb.h
        display/ssd1306.cpp
        display/ssd1306.h
        display/mono_horiz.cpp
        display/mono_horiz.h
        display/epd154.cpp
        display/epd154.h
        display/fonts/gfxfont.h
        display/fonts/FreeMono12pt7b.h
        display/st7789nobuf.cpp
        display/st7789nobuf.h
        display/i2c_device.h
        i2c/PicoI2CDevice.cpp
        i2c/PicoI2CDevice.h
        i2c/PicoI2CBus.cpp
        i2c/PicoI2CBus.h
        display/spi_device.h
        spi/PicoSPIBus.cpp
        spi/PicoSPIBus.h
        spi/PicoSPIDevice.cpp
        spi/PicoSPIDevice.h
        display/rgb_palette.cpp
        display/rgb_palette.h

)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})

# this adds a post build command that tries to copy uf2 to Pico
#add_custom_command(TARGET ${PROJECT_NAME}
#        POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.uf2 d: )

target_include_directories(${PROJECT_NAME} PRIVATE
        modbus
        mqtt
        uart
        i2c
        spi
        display
        display/fonts
        paho.mqtt.embedded-c/MQTTPacket/src
        paho.mqtt.embedded-c/MQTTClient/src
)

# Link to pico_stdlib (gpio, time, etc. functions)
target_link_libraries(${PROJECT_NAME} 
        pico_stdlib
        hardware_i2c
        hardware_spi
        hardware_gpio
        hardware_adc
        pico_cyw43_arch_lwip_poll
        cjson
)
add_library(cjson STATIC cJSON/cJSON.c)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexceptions")

# Enable usb output, disable uart output
pico_enable_stdio_usb(${PROJECT_NAME} 0)
pico_enable_stdio_uart(${PROJECT_NAME} 1)