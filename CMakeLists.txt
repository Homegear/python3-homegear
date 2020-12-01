cmake_minimum_required(VERSION 3.9)
project(python3-homegear)

set(CMAKE_CXX_STANDARD 11)

include_directories(/usr/include/python3.9 /usr/include/python3.8 /usr/include/python3.7 /usr/include/python3.6 /usr/include/python3.5)

add_library(homegear SHARED
        homegear.cpp
        IpcClient.cpp
        IpcClient.h
        PythonVariableConverter.cpp
        PythonVariableConverter.h
        )

target_link_libraries(homegear homegear-ipc)
