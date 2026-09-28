// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15

Menu {
    id: root

    property var items: []
    property var controller: null
    property int sourceItemId: -1
    property var dynamicEntries: []

    function clearDynamicEntries() {
        for (let index = dynamicEntries.length - 1; index >= 0; --index) {
            const entry = dynamicEntries[index]
            if (entry.isMenu) {
                root.removeMenu(entry.object)
            } else {
                root.removeItem(entry.object)
            }
            entry.object.destroy()
        }
        dynamicEntries = []
    }

    function rebuild() {
        clearDynamicEntries()

        const sourceItems = items || []
        for (let index = 0; index < sourceItems.length; ++index) {
            const data = sourceItems[index]
            if (!data || data.visible === false) {
                continue
            }

            let object = null
            let isMenu = false

            if (data.separator === true) {
                object = separatorComponent.createObject(root)
            } else {
                const children = data.children || []
                const hasSubmenu = data["children-display"] === "submenu" || children.length > 0
                if (hasSubmenu) {
                    object = submenuComponent.createObject(root, {
                        "title": data.label || "",
                        "items": children,
                        "controller": root.controller,
                        "sourceItemId": data.id,
                        "enabled": data.enabled !== false
                    })
                    isMenu = true
                } else {
                    object = itemComponent.createObject(root, {
                        "entry": data,
                        "controller": root.controller
                    })
                }
            }

            if (!object) {
                continue
            }

            if (isMenu) {
                root.addMenu(object)
            } else {
                root.addItem(object)
            }

            dynamicEntries.push({
                "object": object,
                "isMenu": isMenu
            })
        }
    }

    onItemsChanged: rebuild()
    onControllerChanged: rebuild()

    onAboutToShow: {
        if (controller && sourceItemId >= 0) {
            controller.prepareSubmenu(sourceItemId)
        }
    }

    Component.onCompleted: rebuild()

    Component {
        id: separatorComponent

        MenuSeparator {
        }
    }

    Component {
        id: itemComponent

        MenuItem {
            required property var entry
            required property var controller

            text: entry.label || ""
            enabled: entry.enabled !== false
            checkable: entry["toggle-type"] === "checkmark"
                       || entry["toggle-type"] === "radio"
            checked: checkable && entry["toggle-state"] === 1

            onTriggered: {
                if (controller) {
                    controller.triggerMenuAction(entry.id, 0)
                }
            }
        }
    }

    Component {
        id: submenuComponent

        DbusMenu {
        }
    }
}
