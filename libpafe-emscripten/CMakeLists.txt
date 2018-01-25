cmake_minimum_required(VERSION 3.5)

project(pafe)

ADD_DEFINITIONS(-DHAVE_CONFIG_H -DDEBUG)

set(LIBRARY_JS "${CMAKE_CURRENT_SOURCE_DIR}/src/webpafe/library.js")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 --js-library ${LIBRARY_JS}")

option(HAVE_LIBUSB_1 "Have libusb 1.x" OFF)
option(HAVE_WEBUSB "Have WebUSB" ON)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config_cmake.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)

include_directories("${PROJECT_BINARY_DIR}" src/libpafe/src)

add_library(pafe STATIC
	src/webpafe/pasori_command.c
	src/libpafe/src/felica_command.c)
