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
    globalmenucontroller.cpp
    globalmenucontroller.h
)

set_target_properties(dgm-core PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(dgm-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(dgm-core PUBLIC Qt6::Core Qt6::DBus)
