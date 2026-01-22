cmake_minimum_required(VERSION 3.16)
project(helloworld C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    src/core.c
    src/tunnel.c
    src/network.c
    src/config.c
    src/stealth.c
)

if(WIN32)
    list(APPEND SOURCES src/gui_win32.cpp)
    # Add resource file if it exists
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/helloworld.rc")
        list(APPEND SOURCES src/helloworld.rc)
    endif()
    add_executable(helloworld WIN32 ${SOURCES})
    target_link_libraries(helloworld ws2_32 wininet shell32 comctl32 gdiplus)
    target_compile_definitions(helloworld PRIVATE _CRT_SECURE_NO_WARNINGS)
    # Math library not needed on Windows (included in MSVC runtime)
elseif(APPLE)
    list(APPEND SOURCES src/gui_macos.m)
    add_executable(helloworld MACOSX_BUNDLE ${SOURCES})
    target_link_libraries(helloworld "-framework Cocoa")
    set_target_properties(helloworld PROPERTIES
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.helloworld.client"
        MACOSX_BUNDLE_BUNDLE_NAME "HelloWorld"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0.0"
    )
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK4 REQUIRED gtk4)
    list(APPEND SOURCES src/gui_linux.c)
    add_executable(helloworld ${SOURCES})
    target_include_directories(helloworld PRIVATE ${GTK4_INCLUDE_DIRS})
    target_link_libraries(helloworld ${GTK4_LIBRARIES})
endif()

target_include_directories(helloworld PRIVATE include)

