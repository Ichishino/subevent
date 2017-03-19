cmake_minimum_required(VERSION 2.8)

project(udp_sender)

include_directories(../../inc)	
add_definitions("-Wall -std=c++11")
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} -pthread)
