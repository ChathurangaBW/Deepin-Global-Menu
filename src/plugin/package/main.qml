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
        spacing: 10

        Label {
            visible: !Applet.menuReady
            text: Applet.hasMenu ? "Global Menu · loading" : "Global Menu · waiting"
            font.bold: true
        }

        Repeater {
            model: Applet.menuItems

            delegate: Label {
                required property var modelData
                visible: modelData.visible !== false && modelData.separator !== true
                enabled: modelData.enabled !== false
                text: modelData.label
                opacity: enabled ? 1.0 : 0.45
                font.bold: false
            }
        }

        Label {
            visible: Applet.menuReady && Applet.menuItems.length === 0
            text: "Global Menu · empty"
            opacity: 0.65
        }
    }

    ToolTip.visible: hover.containsMouse
    ToolTip.text: Applet.menuError.length > 0
                  ? Applet.menuError
                  : (Applet.hasMenu
                     ? ("DBusMenu revision " + Applet.menuRevision + " · " + Applet.menuObjectPath)
                     : "Waiting for active-window tracking to select a registered menu")

    HoverHandler {
        id: hover
    }
}
