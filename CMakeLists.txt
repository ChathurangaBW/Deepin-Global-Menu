cmake_minimum_required(VERSION 3.16)

project(deepin-global-menu
    VERSION 0.3.0
    DESCRIPTION "Native global application menu for modern Deepin/DDE"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

include(GNUInstallDirs)

find_package(Qt6 REQUIRED COMPONENTS Core DBus)

option(DGM_BUILD_PLUGIN "Build the DDE Shell plugin" ON)
option(DGM_BUILD_TOOLS "Build diagnostic tools" ON)
option(DGM_BUILD_TESTS "Build unit tests" ON)
option(DGM_ENABLE_X11_TRACKER "Build the X11 active-window tracker when XCB is available" ON)

if(DGM_ENABLE_X11_TRACKER AND UNIX AND NOT APPLE)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(XCB QUIET IMPORTED_TARGET xcb)
    endif()
endif()

add_subdirectory(src/core)

if(DGM_BUILD_PLUGIN)
    find_package(Qt6 REQUIRED COMPONENTS Qml Quick)
    find_package(DDEShell CONFIG REQUIRED)
    add_subdirectory(src/plugin)
endif()

if(DGM_BUILD_TOOLS)
    add_subdirectory(src/tools)
endif()

if(DGM_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
