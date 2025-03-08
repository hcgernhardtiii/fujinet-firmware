# Figure out where we live
if(WIN32)
	set(USERHOME $ENV{USERPROFILE})
else()
	set(USERHOME $ENV{HOME})
endif()

# Build environment standards and minimum versions
cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(sdkVersion 2.0.0)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.0.0)

# Set the SDK location and location helpers
set(PICO_SDK_PATH "${USERHOME}/.pico-sdk/sdk/${sdkVersion}")
set(PICO_TOOLCHAIN_PATH "${USERHOME}/.pico-sdk/toolchain/${toolchainVersion}")
if (sdkVersion VERSION_LESS "2.0.0")
	# Yes, we set our version at 2.0.0.  If we change that, though, we need
	# this stuff.
	if(WIN32)
		set(pico-sdk-tools_DIR "${USERHOME}/.pico-sdk/tools/${sdkVersion}")
		include(${pico-sdk-tools_DIR}/pico-sdk-tools-config.cmake)
		include(${pico-sdk-tools_DIR}/pcio-sdk-tools-config-version.cmake)
	endif()
else()
	# This is what we'll use for versions 2.0.0 and up, apparently
	set(pioasm_HINT "${USERHOME}/.pico-sdk/tools/${sdkVersion}/pioasm")
	if(EXISTS ${pioasm_HINT})
		set(pioasm_DIR ${pioasm_HINT})
	endif()
	set(picotool_HINT "${USERHOME}/.pico-sdk/picotool/${picotoolVersion}/picotool")
	if(EXISTS ${picotool_HINT})
		set(picotool_DIR ${picotool_HINT})
	endif()
	if(PICO_TOOLCHAIN_PATH MATCHES "RISCV")
		# I don't think we're using this, but in case we ever do:
		set(PICO_PLATFORM rp2350-riscv CACHE STRING "Pico Platform")
	endif()
endif()

# Grab the pico SDK initialization things
include("pico_sdk_import.cmake")

# Project declaration and language types
project(coco-cart-test C CXX ASM)

# Set board type
set(PICO_BOARD pico)

# Initialize the SDK
pico_sdk_init()

# Define the executable
add_executable(coco-cart-test main.c rom.c)

# Generate the .h files we need from the .pio files
pico_generate_pio_header(coco-cart-test ${CMAKE_CURRENT_LIST_DIR}/rom.pio)
pico_generate_pio_header(coco-cart-test ${CMAKE_CURRENT_LIST_DIR}/becker.pio)

# Where is STDIO routed?
# - parameter 1 is the targeted library
# - parameter 2 is 1 if we're using that interface, 0 if we're not
pico_enable_stdio_usb(coco-cart-test 1)
pico_enable_stdio_uart(coco-cart-test 0)

# What libraries are we using? List them here.
target_link_libraries (coco-cart-test pico_stdlib hardware_pio hardware_dma)

# Add the standard include files to the build
target_include_directories(coco-cart-test PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/.. # for our common lwipopts or any other standard includes, if required
)

# Generate the droppable .uf2.  This includes map, bin, and hex files
pico_add_extra_outputs(coco-cart-test)