cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(project2)

zephyr_include_directories(rtdb) #Add this line
target_include_directories(app PRIVATE src/rtdb) #Add this line

zephyr_include_directories(uart) #Add this line
target_include_directories(app PRIVATE src/uart) #Add this line

target_sources(app PRIVATE src/main.c)

target_sources(app PRIVATE src/rtdb/rtdb.c) # Add module c source

target_sources(app PRIVATE src/uart/uart.c) # Add module c source
