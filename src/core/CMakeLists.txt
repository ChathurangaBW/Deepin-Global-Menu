# SPDX-License-Identifier: GPL-3.0-or-later

add_library(dgm-core STATIC
    menuregistry.cpp
    menuregistry.h
    appmenuregistrar.cpp
    appmenuregistrar.h
    dbusmenutypes.cpp
    dbusmenutypes.h
    dbusmenuimporter.cpp
    dbusmenuimporter.h
    gtkmenutypes.cpp
    gtkmenutypes.h
    gtkmenuimporter.cpp
    gtkmenuimporter.h
    activewindowtracker.cpp
    activewindowtracker.h
    globalmenucontroller.cpp
    globalmenucontroller.h
)

if(TARGET PkgConfig::XCB)
    target_sources(dgm-core PRIVATE
        x11activewindowtracker.cpp
        x11activewindowtracker.h
    )
    target_link_libraries(dgm-core PRIVATE PkgConfig::XCB)
    target_compile_definitions(dgm-core PRIVATE DGM_HAVE_X11_TRACKER=1)
endif()

set_target_properties(dgm-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(dgm-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dgm-core PUBLIC Qt6::Core Qt6::DBus)
