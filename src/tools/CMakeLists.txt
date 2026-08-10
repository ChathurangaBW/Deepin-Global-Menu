# SPDX-License-Identifier: GPL-3.0-or-later

add_executable(dgm-inspect
    dgm-inspect.cpp
)

target_link_libraries(dgm-inspect PRIVATE
    dgm-core
    Qt6::Core
    Qt6::DBus
)

install(TARGETS dgm-inspect RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
