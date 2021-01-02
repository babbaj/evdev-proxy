cmake_minimum_required(VERSION 3.17)
project(evdev_joiner)

set(CMAKE_CXX_STANDARD 20)

include_directories(/usr/include/libevdev-1.0/)
link_libraries(/usr/lib/x86_64-linux-gnu/libevdev.a)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(evdev_joiner main.cpp)