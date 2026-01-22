cmake_minimum_required(VERSION 3.16)
project(helloworld VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_CLIENT "Build the client application" ON)
option(BUILD_SERVER "Build the server application" ON)

if(BUILD_CLIENT)
    add_subdirectory(client)
endif()

if(BUILD_SERVER AND NOT WIN32)
    add_subdirectory(server)
endif()

