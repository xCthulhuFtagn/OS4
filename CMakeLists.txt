cmake_minimum_required(VERSION 3.0.0)
project(scrambler VERSION 0.1.0)

add_executable(scrambler scrambler.c)
add_executable(printer printer.c)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
