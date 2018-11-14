cmake_minimum_required(VERSION 2.8)

project(web_socket_server)

include_directories(../../inc)	
add_definitions("-Wall -std=c++17")
add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} -pthread)

# OpenSSL
find_package(PkgConfig REQUIRED)
pkg_search_module(OPENSSL REQUIRED openssl)
if (OPENSSL_FOUND)
    include_directories(${OPENSSL_INCLUDE_DIRS})
    message(STATUS "OpenSSL: ${OPENSSL_VERSION}")
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES})
else ()
    message(STATUS "OpenSSL: @@@ Not Found @@@")
endif ()

