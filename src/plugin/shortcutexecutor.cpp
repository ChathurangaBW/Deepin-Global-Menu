// SPDX-License-Identifier: GPL-3.0-or-later

#include "shortcutexecutor.h"

#include <QDateTime>
#include <QDebug>
#include <QGuiApplication>
#include <QTemporaryFile>
#include <QTimer>
#include <QtGui/qguiapplication_platform.h>

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iterator>

#ifdef DGM_HAVE_XTEST
#include <X11/Xlib.h>
#include <X11/extensions/XTest.h>
#include <X11/keysym.h>
#endif

#ifdef DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD
#include "qwayland-virtual-keyboard-unstable-v1.h"

#include <QtWaylandClient/QWaylandClientExtension>
#include <wayland-client-core.h>
#include <wayland-client-protocol.h>
#include <xkbcommon/xkbcommon.h>
#endif

namespace {

struct ShortcutParts
{
    bool control = false;
    bool alt = false;
    bool shift = false;
    bool meta = false;
    QString key;

    [[nodiscard]] bool isValid() const
    {
        return !key.isEmpty();
    }
};

ShortcutParts parseShortcut(QString shortcut)
{
    shortcut = shortcut.trimmed();
    shortcut.replace(QStringLiteral("<Ctrl>"), QStringLiteral("Ctrl+"),
                     Qt::CaseInsensitive);
    shortcut.replace(QStringLiteral("<Alt>"), QStringLiteral("Alt+"),
                     Qt::CaseInsensitive);
    shortcut.replace(QStringLiteral("<Shift>"), QStringLiteral("Shift+"),
                     Qt::CaseInsensitive);
    shortcut.replace(QStringLiteral("<Super>"), QStringLiteral("Meta+"),
                     Qt::CaseInsensitive);

    const auto tokens = shortcut.split(QLatin1Char('+'), Qt::SkipEmptyParts);
    ShortcutParts result;
    if (tokens.isEmpty()) {
        return result;
    }

    for (int i = 0; i < tokens.size() - 1; ++i) {
        const QString token = tokens.at(i).trimmed().toLower();
        if (token == QStringLiteral("ctrl") || token == QStringLiteral("control")) {
            result.control = true;
        } else if (token == QStringLiteral("alt")) {
            result.alt = true;
        } else if (token == QStringLiteral("shift")) {
            result.shift = true;
        } else if (token == QStringLiteral("meta")
                   || token == QStringLiteral("super")
                   || token == QStringLiteral("win")) {
            result.meta = true;
        } else {
            return {};
        }
    }

    result.key = tokens.constLast().trimmed();
    if (result.key.compare(QStringLiteral("Space"), Qt::CaseInsensitive) == 0) {
        result.key = QStringLiteral("space");
    } else if (result.key.compare(QStringLiteral("Esc"), Qt::CaseInsensitive) == 0) {
        result.key = QStringLiteral("Escape");
    } else if (result.key.compare(QStringLiteral("PageUp"), Qt::CaseInsensitive) == 0) {
        result.key = QStringLiteral("Page_Up");
    } else if (result.key.compare(QStringLiteral("PageDown"), Qt::CaseInsensitive) == 0) {
        result.key = QStringLiteral("Page_Down");
    }

    return result;
}

#ifdef DGM_HAVE_XTEST
KeySym x11KeysymForName(QString key)
{
    if (key.size() == 1 && key.at(0).isLetter()) {
        key = key.toLower();
    }
    const QByteArray name = key.toLatin1();
    KeySym symbol = XStringToKeysym(name.constData());
    if (symbol == NoSymbol && key.size() == 1) {
        symbol = static_cast<KeySym>(key.at(0).unicode());
    }
    return symbol;
}
#endif

#ifdef DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD
xkb_keysym_t xkbKeysymForName(QString key)
{
    if (key.size() == 1 && key.at(0).isLetter()) {
        key = key.toLower();
    }

    if (key.size() == 1) {
        return xkb_utf32_to_keysym(key.at(0).unicode());
    }

    const QByteArray name = key.toLatin1();
    return xkb_keysym_from_name(name.constData(), XKB_KEYSYM_CASE_INSENSITIVE);
}

xkb_keycode_t keycodeForSymbol(xkb_keymap *keymap, xkb_keysym_t symbol)
{
    if (!keymap || symbol == XKB_KEY_NoSymbol) {
        return XKB_KEYCODE_INVALID;
    }

    const xkb_keycode_t minCode = xkb_keymap_min_keycode(keymap);
    const xkb_keycode_t maxCode = xkb_keymap_max_keycode(keymap);
    for (xkb_keycode_t code = minCode; code <= maxCode; ++code) {
        for (xkb_layout_index_t layout = 0;
             layout < xkb_keymap_num_layouts_for_key(keymap, code);
             ++layout) {
            const xkb_level_index_t levels =
                xkb_keymap_num_levels_for_key(keymap, code, layout);
            for (xkb_level_index_t level = 0; level < levels; ++level) {
                const xkb_keysym_t *symbols = nullptr;
                const int count = xkb_keymap_key_get_syms_by_level(
                    keymap, code, layout, level, &symbols);
                for (int index = 0; index < count; ++index) {
                    if (symbols[index] == symbol) {
                        return code;
                    }
                }
            }
        }
    }
    return XKB_KEYCODE_INVALID;
}

uint32_t modifierMask(xkb_keymap *keymap, const ShortcutParts &parts)
{
    uint32_t mask = 0;
    auto addModifier = [keymap, &mask](const char *name) {
        const xkb_mod_index_t index = xkb_keymap_mod_get_index(keymap, name);
        if (index != XKB_MOD_INVALID && index < 32) {
            mask |= (1u << index);
        }
    };

    if (parts.control) {
        addModifier(XKB_MOD_NAME_CTRL);
    }
    if (parts.alt) {
        addModifier(XKB_MOD_NAME_ALT);
    }
    if (parts.shift) {
        addModifier(XKB_MOD_NAME_SHIFT);
    }
    if (parts.meta) {
        addModifier(XKB_MOD_NAME_LOGO);
    }
    return mask;
}
#endif

} // namespace

#ifdef DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD
class WaylandVirtualKeyboardManager final
    : public QWaylandClientExtensionTemplate<WaylandVirtualKeyboardManager>
    , public QtWayland::zwp_virtual_keyboard_manager_v1
{
public:
    WaylandVirtualKeyboardManager()
        : QWaylandClientExtensionTemplate<WaylandVirtualKeyboardManager>(1)
    {
    }
};
#endif

ShortcutExecutor::ShortcutExecutor(QObject *parent)
    : QObject(parent)
{
#ifdef DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD
    if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
        m_waylandManager = new WaylandVirtualKeyboardManager();
        m_waylandManager->setParent(this);
        connect(m_waylandManager, &QWaylandClientExtension::activeChanged,
                this, &ShortcutExecutor::availabilityChanged);
    }
#endif
}

ShortcutExecutor::~ShortcutExecutor() = default;

bool ShortcutExecutor::available() const
{
    const QString platform = QGuiApplication::platformName();

#ifdef DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD
    if (platform == QStringLiteral("wayland")) {
        return m_waylandManager && m_waylandManager->isActive();
    }
#endif

#ifdef DGM_HAVE_XTEST
    if (platform == QStringLiteral("xcb")) {
        Display *display = XOpenDisplay(nullptr);
        if (!display) {
            return false;
        }

        int eventBase = 0;
        int errorBase = 0;
        int majorVersion = 0;
        int minorVersion = 0;
        const bool supported = XTestQueryExtension(display,
                                                    &eventBase,
                                                    &errorBase,
                                                    &majorVersion,
                                                    &minorVersion);
        XCloseDisplay(display);
        return supported;
    }
#endif

    return false;
}

void ShortcutExecutor::send(const QString &shortcut)
{
    // QML closes the popup after activation. Give the shell one event-loop
    // turn plus a small margin so keyboard focus is back on the target app.
    QTimer::singleShot(75, this, [this, shortcut] {
        sendNow(shortcut);
    });
}

void ShortcutExecutor::sendNow(const QString &shortcut)
{
    bool sent = false;
    const QString platform = QGuiApplication::platformName();

    if (platform == QStringLiteral("wayland")) {
        sent = sendWayland(shortcut);
    } else if (platform == QStringLiteral("xcb")) {
        sent = sendX11(shortcut);
    }

    if (!sent) {
        qWarning() << "Deepin Global Menu: could not send shortcut" << shortcut
                   << "on platform" << platform;
    }
}

bool ShortcutExecutor::sendX11(const QString &shortcut)
{
#ifdef DGM_HAVE_XTEST
    const ShortcutParts parts = parseShortcut(shortcut);
    if (!parts.isValid()) {
        return false;
    }

    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        return false;
    }

    int eventBase = 0;
    int errorBase = 0;
    int majorVersion = 0;
    int minorVersion = 0;
    if (!XTestQueryExtension(display,
                             &eventBase,
                             &errorBase,
                             &majorVersion,
                             &minorVersion)) {
        XCloseDisplay(display);
        return false;
    }

    auto keycodeFor = [display](KeySym symbol) -> KeyCode {
        return symbol == NoSymbol ? 0 : XKeysymToKeycode(display, symbol);
    };

    const KeyCode control = parts.control ? keycodeFor(XK_Control_L) : 0;
    const KeyCode alt = parts.alt ? keycodeFor(XK_Alt_L) : 0;
    const KeyCode shift = parts.shift ? keycodeFor(XK_Shift_L) : 0;
    const KeyCode meta = parts.meta ? keycodeFor(XK_Super_L) : 0;
    const KeyCode key = keycodeFor(x11KeysymForName(parts.key));

    if (key == 0
        || (parts.control && control == 0)
        || (parts.alt && alt == 0)
        || (parts.shift && shift == 0)
        || (parts.meta && meta == 0)) {
        XCloseDisplay(display);
        return false;
    }

    bool success = true;
    const KeyCode modifiers[] = {control, alt, shift, meta};
    for (const KeyCode modifier : modifiers) {
        if (modifier != 0) {
            success = XTestFakeKeyEvent(display,
                                        modifier,
                                        True,
                                        CurrentTime) && success;
        }
    }

    success = XTestFakeKeyEvent(display, key, True, CurrentTime) && success;
    success = XTestFakeKeyEvent(display, key, False, CurrentTime) && success;

    for (auto it = std::rbegin(modifiers); it != std::rend(modifiers); ++it) {
        if (*it != 0) {
            success = XTestFakeKeyEvent(display,
                                        *it,
                                        False,
                                        CurrentTime) && success;
        }
    }

    XFlush(display);
    XCloseDisplay(display);
    return success;
#else
    Q_UNUSED(shortcut);
    return false;
#endif
}

bool ShortcutExecutor::sendWayland(const QString &shortcut)
{
#ifdef DGM_HAVE_WAYLAND_VIRTUAL_KEYBOARD
    if (!m_waylandManager || !m_waylandManager->isActive()) {
        return false;
    }

    const ShortcutParts parts = parseShortcut(shortcut);
    if (!parts.isValid()) {
        return false;
    }

    auto *native = qGuiApp
        ? qGuiApp->nativeInterface<QNativeInterface::QWaylandApplication>()
        : nullptr;
    if (!native || !native->seat() || !native->display()) {
        return false;
    }

    xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    xkb_keymap *keymap = context
        ? xkb_keymap_new_from_names(context, nullptr, XKB_KEYMAP_COMPILE_NO_FLAGS)
        : nullptr;
    if (!keymap) {
        if (context) {
            xkb_context_unref(context);
        }
        return false;
    }

    char *keymapText = xkb_keymap_get_as_string(
        keymap, XKB_KEYMAP_FORMAT_TEXT_V1);
    if (!keymapText) {
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        return false;
    }

    const xkb_keysym_t symbol = xkbKeysymForName(parts.key);
    const xkb_keycode_t xkbCode = keycodeForSymbol(keymap, symbol);
    if (xkbCode == XKB_KEYCODE_INVALID || xkbCode < 8) {
        std::free(keymapText);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        return false;
    }

    QTemporaryFile keymapFile;
    if (!keymapFile.open()) {
        std::free(keymapText);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        return false;
    }

    const qint64 keymapSize = static_cast<qint64>(std::strlen(keymapText)) + 1;
    if (keymapFile.write(keymapText, keymapSize) != keymapSize
        || !keymapFile.flush()) {
        std::free(keymapText);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        return false;
    }
    keymapFile.seek(0);

    auto *object = m_waylandManager->create_virtual_keyboard(native->seat());
    if (!object) {
        std::free(keymapText);
        xkb_keymap_unref(keymap);
        xkb_context_unref(context);
        return false;
    }

    QtWayland::zwp_virtual_keyboard_v1 keyboard(object);
    keyboard.keymap(WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
                    keymapFile.handle(),
                    static_cast<uint32_t>(keymapSize));

    const uint32_t modifiers = modifierMask(keymap, parts);
    keyboard.modifiers(modifiers, 0, 0, 0);

    const uint32_t evdevCode = static_cast<uint32_t>(xkbCode - 8);
    const uint32_t timestamp =
        static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch());

    keyboard.key(timestamp, evdevCode, WL_KEYBOARD_KEY_STATE_PRESSED);
    keyboard.key(timestamp, evdevCode, WL_KEYBOARD_KEY_STATE_RELEASED);
    keyboard.modifiers(0, 0, 0, 0);
    wl_display_flush(native->display());
    keyboard.destroy();

    std::free(keymapText);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    return true;
#else
    Q_UNUSED(shortcut);
    return false;
#endif
}
