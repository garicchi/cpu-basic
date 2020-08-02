cmake_minimum_required(VERSION 3.17)
project(emulator)

set(CMAKE_CXX_STANDARD 14)

add_executable(emulator main.cpp alu.hpp psw.hpp memory.hpp inst.hpp cpu.hpp)