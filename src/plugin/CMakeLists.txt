# SPDX-License-Identifier: GPL-3.0-or-later

add_library(ds-global-menu SHARED
    ddeactivewindowtracker.cpp
    ddeactivewindowtracker.h
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

ds_install_package(PACKAGE org.deepin.ds.globalmenu TARGET ds-global-menu)
