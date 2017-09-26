cmake_minimum_required(VERSION 3.8)
project(dbvpnclient)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp dbvpnclient.h dbvpnclient.cpp)
add_executable(dbvpnclient ${SOURCE_FILES})

#include_directories(/usr/local/include)
#link_directories(/usr/local/lib)

LINK_LIBRARIES(/usr/local/lib)
TARGET_LINK_LIBRARIES(dbvpnclient tins)