// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.ds 1.0

AppletItem {
    id: root

    implicitWidth: content.implicitWidth + 20
    implicitHeight: 30

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 10
        anchors.rightMargin: 10
        spacing: 8

        Label {
            text: Applet.hasMenu ? "Global Menu" : "Global Menu · waiting"
            font.bold: true
        }

        Label {
            visible: Applet.hasMenu
            text: Applet.menuService
            opacity: 0.65
            elide: Text.ElideRight
            Layout.maximumWidth: 260
        }
    }

    ToolTip.visible: hover.containsMouse
    ToolTip.text: Applet.hasMenu
                  ? ("Registered menu: " + Applet.menuObjectPath)
                  : "Waiting for an active-window menu registration"

    HoverHandler {
        id: hover
    }
}
