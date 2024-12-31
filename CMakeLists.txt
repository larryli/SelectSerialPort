cmake_minimum_required(VERSION 3.5.0)
project(SelectSerialPort VERSION 0.1.0 LANGUAGES C)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
add_executable(SelectSerialPort WIN32 main.c SerialPort.c main.rc)
target_link_libraries(SelectSerialPort comctl32.lib comdlg32.lib)
