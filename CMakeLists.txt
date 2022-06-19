cmake_minimum_required(VERSION 3.20.0)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
AUX_SOURCE_DIRECTORY(src SOURCES)
project(morse_live VERSION 1.0.0 DESCRIPTION "Live time-based morse code decoding")

add_executable(morse_live ${SOURCES})