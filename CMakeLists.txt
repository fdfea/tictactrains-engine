cmake_minimum_required(VERSION 3.10)

project(tictactrains)

enable_language(C)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -O3")
#add_definitions()

file(GLOB SOURCES src/*.c)

add_executable(tictactrains ${SOURCES})

target_link_libraries(tictactrains m)

configure_file(${CMAKE_SOURCE_DIR}/src/ttt.conf ${CMAKE_BINARY_DIR}/ttt.conf COPYONLY)
