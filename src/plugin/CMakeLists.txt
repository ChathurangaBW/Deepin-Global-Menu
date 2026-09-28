# SPDX-License-Identifier: GPL-3.0-or-later

add_library(ds-global-menu SHARED
    ddeactivewindowtracker.cpp
    ddeactivewindowtracker.h
    ddefallbackactions.cpp
    ddefallbackactions.h
    shortcutexecutor.cpp
    shortcutexecutor.h
    globalmenuapplet.cpp
    globalmenuapplet.h
)

target_link_libraries(ds-global-menu PRIVATE
    Dde::Shell
    dgm-core
    Qt6::Core
    Qt6::DBus
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

if(TARGET Qt6::WaylandClient AND TARGET PkgConfig::XKBCOMMON)
    qt_generate_wayland_protocol_client_sources(ds-global-menu
        FILES
            ${PROJECT_SOURCE_DIR}/protocols/virtual-keyboard-unstable-v1.xml
    )
    target_link_libraries(ds-global-menu PRIVATE
        Qt6::WaylandClient
        PkgConfig::XKBCOMMON
    )
    target_compile_definitions(ds-global-menu PRIVATE
        DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD=1
    )
endif()

if(TARGET PkgConfig::XTEST)
    target_link_libraries(ds-global-menu PRIVATE PkgConfig::XTEST)
    target_compile_definitions(ds-global-menu PRIVATE DGM_HAVE_XTEST=1)
endif()

ds_install_package(PACKAGE org.deepin.ds.globalmenu TARGET ds-global-menu)
