cmake_minimum_required(VERSION 3.8)
project(dbvpnServer)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES main.cpp dbvpnserver.h dbvpnserver.cpp)
add_executable(dbvpnServer ${SOURCE_FILES})

#include_directories(/usr/local/include)
#link_directories(/usr/local/lib)

LINK_LIBRARIES(/usr/local/lib)
TARGET_LINK_LIBRARIES(dbvpnServer tins)