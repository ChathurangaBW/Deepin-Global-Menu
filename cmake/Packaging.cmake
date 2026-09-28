# SPDX-License-Identifier: GPL-3.0-or-later
#
# Distribution packages are generated from the normal CMake install tree so
# .deb and .rpm payloads match "cmake --install".

set(CPACK_PACKAGE_NAME "deepin-global-menu")
set(CPACK_PACKAGE_VENDOR "Deepin Global Menu")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_DESCRIPTION
    "Native global application menu for modern Deepin/DDE. "
    "Provides a DDE Shell global-menu applet, DBusMenu and GTK menu importers, "
    "X11/XWayland and Treeland active-application tracking, fallback actions, "
    "and the dgm-inspect diagnostic utility."
)
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/ChathurangaBW/Deepin-Global-Menu")
set(CPACK_PACKAGE_CONTACT "ChathurangaBW")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_CHECKSUM "SHA256")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
set(CPACK_STRIP_FILES ON)

# Deepin/Debian package. dpkg-shlibdeps derives runtime library dependencies
# from the binaries that are actually packaged.
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set(CPACK_DEBIAN_PACKAGE_NAME "deepin-global-menu")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "ChathurangaBW")
set(CPACK_DEBIAN_PACKAGE_SECTION "x11")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)

# Alternate RPM format for compatible DDE/Qt environments.
set(CPACK_RPM_FILE_NAME RPM-DEFAULT)
set(CPACK_RPM_PACKAGE_NAME "deepin-global-menu")
set(CPACK_RPM_PACKAGE_LICENSE "GPL-3.0-or-later")
set(CPACK_RPM_PACKAGE_GROUP "User Interface/Desktops")
set(CPACK_RPM_PACKAGE_URL "${CPACK_PACKAGE_HOMEPAGE_URL}")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)
set(CPACK_RPM_PACKAGE_RELOCATABLE OFF)

include(CPack)
