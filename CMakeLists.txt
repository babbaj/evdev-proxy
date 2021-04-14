cmake_minimum_required(VERSION 3.17)
project(evdev_joiner)

set(CMAKE_CXX_STANDARD 20)

find_package(PkgConfig)

pkg_check_modules(LIBEVDEV REQUIRED IMPORTED_TARGET libevdev)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(evdev_joiner main.cpp)
target_link_libraries(evdev_joiner PkgConfig::LIBEVDEV)