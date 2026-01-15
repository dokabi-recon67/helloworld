cmake_minimum_required(VERSION 3.16)
project(helloworld-server C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(SOURCES
    src/server.c
    src/main.c
)

add_executable(helloworld-server ${SOURCES})
target_include_directories(helloworld-server PRIVATE include)

add_executable(helloworld-setup src/installer_tui.c)

install(TARGETS helloworld-server helloworld-setup DESTINATION /usr/local/bin)

