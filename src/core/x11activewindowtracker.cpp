// SPDX-License-Identifier: GPL-3.0-or-later

#include "x11activewindowtracker.h"

#include <QByteArray>
#include <QSocketNotifier>

#include <cstdlib>

namespace dgm {

namespace {

xcb_screen_t *screenForNumber(xcb_connection_t *connection, int screenNumber)
{
    auto iterator = xcb_setup_roots_iterator(xcb_get_setup(connection));
    for (int index = 0; index < screenNumber && iterator.rem > 0; ++index) {
        xcb_screen_next(&iterator);
    }
    return iterator.rem > 0 ? iterator.data : nullptr;
}

} // namespace

X11ActiveWindowTracker::X11ActiveWindowTracker(QObject *parent)
    : ActiveWindowTracker(parent)
{
}

X11ActiveWindowTracker::~X11ActiveWindowTracker()
{
    stop();
}

bool X11ActiveWindowTracker::start()
{
    if (m_running) {
        return true;
    }

    int screenNumber = 0;
    m_connection = xcb_connect(nullptr, &screenNumber);
    if (!m_connection || xcb_connection_has_error(m_connection) != 0) {
        stop();
        return false;
    }

    const auto *screen = screenForNumber(m_connection, screenNumber);
    if (!screen) {
        stop();
        return false;
    }
    m_rootWindow = screen->root;

    m_activeWindowAtom = internAtom("_NET_ACTIVE_WINDOW");
    m_netWmPidAtom = internAtom("_NET_WM_PID");
    m_netWmNameAtom = internAtom("_NET_WM_NAME");
    m_gtkApplicationIdAtom = internAtom("_GTK_APPLICATION_ID");
    m_gtkUniqueBusNameAtom = internAtom("_GTK_UNIQUE_BUS_NAME");
    m_gtkApplicationObjectPathAtom = internAtom("_GTK_APPLICATION_OBJECT_PATH");
    m_gtkWindowObjectPathAtom = internAtom("_GTK_WINDOW_OBJECT_PATH");
    m_gtkAppMenuObjectPathAtom = internAtom("_GTK_APP_MENU_OBJECT_PATH");
    m_gtkMenubarObjectPathAtom = internAtom("_GTK_MENUBAR_OBJECT_PATH");

    if (m_activeWindowAtom == XCB_ATOM_NONE) {
        stop();
        return false;
    }

    const uint32_t eventMask = XCB_EVENT_MASK_PROPERTY_CHANGE;
    const auto attributesCookie = xcb_change_window_attributes_checked(
        m_connection,
        m_rootWindow,
        XCB_CW_EVENT_MASK,
        &eventMask);
    if (auto *error = xcb_request_check(m_connection, attributesCookie)) {
        std::free(error);
        stop();
        return false;
    }

    xcb_flush(m_connection);

    m_notifier = new QSocketNotifier(xcb_get_file_descriptor(m_connection),
                                     QSocketNotifier::Read,
                                     this);
    connect(m_notifier, &QSocketNotifier::activated,
            this, [this] { processEvents(); });

    m_running = true;
    refreshActiveWindow();
    return true;
}

void X11ActiveWindowTracker::stop()
{
    delete m_notifier;
    m_notifier = nullptr;

    if (m_connection) {
        xcb_disconnect(m_connection);
        m_connection = nullptr;
    }

    m_rootWindow = XCB_WINDOW_NONE;
    m_activeWindowAtom = XCB_ATOM_NONE;
    m_netWmPidAtom = XCB_ATOM_NONE;
    m_netWmNameAtom = XCB_ATOM_NONE;
    m_gtkApplicationIdAtom = XCB_ATOM_NONE;
    m_gtkUniqueBusNameAtom = XCB_ATOM_NONE;
    m_gtkApplicationObjectPathAtom = XCB_ATOM_NONE;
    m_gtkWindowObjectPathAtom = XCB_ATOM_NONE;
    m_gtkAppMenuObjectPathAtom = XCB_ATOM_NONE;
    m_gtkMenubarObjectPathAtom = XCB_ATOM_NONE;
    m_running = false;
    setActiveWindowInfo({});
}

bool X11ActiveWindowTracker::isRunning() const
{
    return m_running;
}

void X11ActiveWindowTracker::processEvents()
{
    if (!m_connection) {
        return;
    }

    while (auto *event = xcb_poll_for_event(m_connection)) {
        const uint8_t type = event->response_type & ~0x80;
        if (type == XCB_PROPERTY_NOTIFY) {
            const auto *propertyEvent = reinterpret_cast<xcb_property_notify_event_t *>(event);
            if (propertyEvent->window == m_rootWindow
                && propertyEvent->atom == m_activeWindowAtom) {
                refreshActiveWindow();
            }
        }
        std::free(event);
    }

    if (xcb_connection_has_error(m_connection) != 0) {
        stop();
    }
}

void X11ActiveWindowTracker::refreshActiveWindow()
{
    if (!m_connection || m_rootWindow == XCB_WINDOW_NONE
        || m_activeWindowAtom == XCB_ATOM_NONE) {
        setActiveWindowInfo({});
        return;
    }

    const auto cookie = xcb_get_property(m_connection,
                                         false,
                                         m_rootWindow,
                                         m_activeWindowAtom,
                                         XCB_ATOM_WINDOW,
                                         0,
                                         1);

    xcb_generic_error_t *error = nullptr;
    auto *reply = xcb_get_property_reply(m_connection, cookie, &error);
    const bool hadError = error != nullptr;
    std::free(error);

    if (!reply || hadError) {
        std::free(reply);
        setActiveWindowInfo({});
        return;
    }

    xcb_window_t activeWindow = XCB_WINDOW_NONE;
    if (reply->type == XCB_ATOM_WINDOW
        && reply->format == 32
        && xcb_get_property_value_length(reply) >= static_cast<int>(sizeof(xcb_window_t))) {
        activeWindow = *static_cast<xcb_window_t *>(xcb_get_property_value(reply));
    }
    std::free(reply);

    if (activeWindow == XCB_WINDOW_NONE) {
        setActiveWindowInfo({});
        return;
    }

    ActiveWindowInfo info;
    info.nativeId = activeWindow;
    info.x11Id = activeWindow;
    info.pid = readCardinalProperty(activeWindow, m_netWmPidAtom);
    info.appId = readStringProperty(activeWindow, m_gtkApplicationIdAtom);
    if (info.appId.isEmpty()) {
        info.appId = readWmClass(activeWindow);
    }
    info.appName = info.appId;
    info.title = readStringProperty(activeWindow, m_netWmNameAtom);
    if (info.title.isEmpty()) {
        info.title = readStringProperty(activeWindow, XCB_ATOM_WM_NAME);
    }
    info.backend = QStringLiteral("x11");

    info.gtkUniqueBusName = readStringProperty(activeWindow, m_gtkUniqueBusNameAtom);
    info.gtkApplicationObjectPath =
        readStringProperty(activeWindow, m_gtkApplicationObjectPathAtom);
    info.gtkWindowObjectPath =
        readStringProperty(activeWindow, m_gtkWindowObjectPathAtom);
    info.gtkAppMenuObjectPath =
        readStringProperty(activeWindow, m_gtkAppMenuObjectPathAtom);
    info.gtkMenubarObjectPath =
        readStringProperty(activeWindow, m_gtkMenubarObjectPathAtom);

    setActiveWindowInfo(info);
}

xcb_atom_t X11ActiveWindowTracker::internAtom(const char *name) const
{
    if (!m_connection || !name) {
        return XCB_ATOM_NONE;
    }

    const QByteArray bytes(name);
    const auto cookie = xcb_intern_atom(m_connection, false, bytes.size(), bytes.constData());
    auto *reply = xcb_intern_atom_reply(m_connection, cookie, nullptr);
    if (!reply) {
        return XCB_ATOM_NONE;
    }

    const xcb_atom_t atom = reply->atom;
    std::free(reply);
    return atom;
}

QString X11ActiveWindowTracker::readStringProperty(xcb_window_t window, xcb_atom_t atom) const
{
    if (!m_connection || window == XCB_WINDOW_NONE || atom == XCB_ATOM_NONE) {
        return {};
    }

    const auto cookie = xcb_get_property(m_connection,
                                         false,
                                         window,
                                         atom,
                                         XCB_GET_PROPERTY_TYPE_ANY,
                                         0,
                                         4096);
    xcb_generic_error_t *error = nullptr;
    auto *reply = xcb_get_property_reply(m_connection, cookie, &error);
    const bool hadError = error != nullptr;
    std::free(error);

    if (!reply || hadError || reply->format != 8) {
        std::free(reply);
        return {};
    }

    const int length = xcb_get_property_value_length(reply);
    const auto *data = static_cast<const char *>(xcb_get_property_value(reply));
    const QString value = length > 0 ? QString::fromUtf8(data, length) : QString{};
    std::free(reply);
    return value;
}

QString X11ActiveWindowTracker::readWmClass(xcb_window_t window) const
{
    if (!m_connection || window == XCB_WINDOW_NONE) {
        return {};
    }

    const auto cookie = xcb_get_property(m_connection,
                                         false,
                                         window,
                                         XCB_ATOM_WM_CLASS,
                                         XCB_ATOM_STRING,
                                         0,
                                         1024);
    xcb_generic_error_t *error = nullptr;
    auto *reply = xcb_get_property_reply(m_connection, cookie, &error);
    const bool hadError = error != nullptr;
    std::free(error);

    if (!reply || hadError || reply->format != 8) {
        std::free(reply);
        return {};
    }

    const int length = xcb_get_property_value_length(reply);
    const auto *data = static_cast<const char *>(xcb_get_property_value(reply));
    const QByteArray raw = length > 0 ? QByteArray(data, length) : QByteArray{};
    std::free(reply);

    const auto parts = raw.split('\0');
    if (parts.size() > 1 && !parts.at(1).isEmpty()) {
        return QString::fromUtf8(parts.at(1));
    }
    return !parts.isEmpty() ? QString::fromUtf8(parts.constFirst()) : QString{};
}

quint32 X11ActiveWindowTracker::readCardinalProperty(xcb_window_t window, xcb_atom_t atom) const
{
    if (!m_connection || window == XCB_WINDOW_NONE || atom == XCB_ATOM_NONE) {
        return 0;
    }

    const auto cookie = xcb_get_property(m_connection,
                                         false,
                                         window,
                                         atom,
                                         XCB_ATOM_CARDINAL,
                                         0,
                                         1);
    xcb_generic_error_t *error = nullptr;
    auto *reply = xcb_get_property_reply(m_connection, cookie, &error);
    const bool hadError = error != nullptr;
    std::free(error);

    quint32 value = 0;
    if (reply && !hadError
        && reply->format == 32
        && xcb_get_property_value_length(reply) >= static_cast<int>(sizeof(quint32))) {
        value = *static_cast<quint32 *>(xcb_get_property_value(reply));
    }

    std::free(reply);
    return value;
}

} // namespace dgm
