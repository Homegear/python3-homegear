cmake_minimum_required(VERSION 3.9)
project(homegear_python)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include/python3.5 /usr/include/python3.6 /usr/include/python3.7)

add_library(homegear_python SHARED
        homegear.cpp
        IpcClient.cpp
        IpcClient.h
        PythonVariableConverter.cpp
        PythonVariableConverter.h
        )

target_link_libraries(homegear_python homegear-ipc)