# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)
set(PICO_SDK_PATH "/home/phooky/tools/pico/pico-sdk")

set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

if (PICO_SDK_VERSION_STRING VERSION_LESS "1.4.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 1.4.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

project(pico_w_led_panel C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pico_w_led_panel
  pico_w_led_panel.cpp
  matrix/led_matrix.cpp
  ${PICO_SDK_PATH}/lib/lwip/src/apps/mqtt/mqtt.c)

add_definitions(
-DSNTP_SERVER_DNS=1
-DSNTP_SERVER_ADDRESS="pool.ntp.org"
-DSNTP_SET_SYSTEM_TIME_NTP=set_system_time
-DSNTP_STARTUP_DELAY=0
)

if(DEFINED ENV{PW_SSID})
  target_compile_definitions(pico_w_led_panel PRIVATE PW_SSID=$ENV{PW_SSID})
endif()
if(DEFINED ENV{PW_PASS})
  target_compile_definitions(pico_w_led_panel PRIVATE PW_PASS=$ENV{PW_PASS})
endif()
if(DEFINED ENV{TCP_PORT})
  target_compile_definitions(pico_w_led_panel PRIVATE TCP_PORT=$ENV{TCP_PORT})
endif()
if(DEFINED ENV{IP_ADDR})
  target_compile_definitions(pico_w_led_panel PRIVATE IP_ADDR=$ENV{IP_ADDR})
endif()
if(DEFINED ENV{MQTT_HOSTNAME})
  target_compile_definitions(pico_w_led_panel PRIVATE MQTT_HOSTNAME=$ENV{MQTT_HOSTNAME})
endif()

pico_set_program_name(pico_w_led_panel "pico_w_led_panel")
pico_set_program_version(pico_w_led_panel "0.1")

pico_enable_stdio_uart(pico_w_led_panel 1)
pico_enable_stdio_usb(pico_w_led_panel 1)

# Add the standard library to the build
target_link_libraries(pico_w_led_panel
  pico_cyw43_arch_lwip_threadsafe_background
  pico_stdlib
  pico_multicore
  pico_lwip_sntp
  hardware_rtc)

# Add the standard include files to the build
target_include_directories(pico_w_led_panel PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

pico_add_extra_outputs(pico_w_led_panel)

