cmake_minimum_required(VERSION 3.0)

#set(PROJECT_VERSION 0.0.1)
project(OS1)# VERSION ${PROJECT_VERSION})

add_executable(scrambler scrambler.c)
add_executable(printer printer.c)

set_target_properties(
    scrambler PROPERTIES
    GCC_VERSION 11
    GCC_VERSION_REQUIREMENTS ON
)

set_target_properties(
    printer PROPERTIES
    GCC_VERSION 11
    GCC_VERSION_REQUIREMENTS ON
)