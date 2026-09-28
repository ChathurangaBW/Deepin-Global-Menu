// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import org.deepin.ds 1.0

AppletItem {
    id: root

    implicitWidth: content.implicitWidth + 16
    implicitHeight: 30

    RowLayout {
        id: content
        anchors.fill: parent
        anchors.leftMargin: 8
        anchors.rightMargin: 8
        spacing: 2

        Label {
            visible: !Applet.menuReady
            text: Applet.hasMenu ? "Global Menu · loading" : "Global Menu · waiting"
            font.bold: true
        }

        Repeater {
            model: Applet.menuItems

            delegate: ToolButton {
                id: menuButton

                required property var modelData

                visible: modelData.visible !== false && modelData.separator !== true
                enabled: modelData.enabled !== false
                text: modelData.label || ""
                flat: true
                focusPolicy: Qt.StrongFocus
                Layout.preferredHeight: 28

                onClicked: {
                    const children = modelData.children || []
                    const hasSubmenu = modelData["children-display"] === "submenu"
                                       || children.length > 0

                    if (hasSubmenu) {
                        submenu.popup(menuButton)
                    } else {
                        Applet.triggerMenuAction(modelData.id, 0)
                    }
                }

                DbusMenu {
                    id: submenu
                    title: menuButton.text
                    items: menuButton.modelData.children || []
                    controller: Applet
                    sourceItemId: menuButton.modelData.id
                }
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
                     ? ("Window 0x"
                        + Applet.activeWindowId.toString(16)
                        + " · "
                        + Applet.menuSource
                        + " revision "
                        + Applet.menuRevision)
                     : "Waiting for the active application's exported menu")

    HoverHandler {
        id: hover
    }
}
