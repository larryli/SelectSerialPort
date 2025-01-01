cmake_minimum_required(VERSION 3.5.0)
project(SelectSerialPort VERSION 0.1.0 LANGUAGES C)

if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
add_executable(SelectSerialPort WIN32 main.c SerialPort.c main.rc)
target_link_libraries(SelectSerialPort comctl32 comdlg32)
