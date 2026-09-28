// SPDX-License-Identifier: GPL-3.0-or-later

#include "x11activewindowtracker.h"

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

    constexpr char atomName[] = "_NET_ACTIVE_WINDOW";
    const auto atomCookie = xcb_intern_atom(m_connection, false, sizeof(atomName) - 1, atomName);
    auto *atomReply = xcb_intern_atom_reply(m_connection, atomCookie, nullptr);
    if (!atomReply) {
        stop();
        return false;
    }
    m_activeWindowAtom = atomReply->atom;
    std::free(atomReply);

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
    m_running = false;
    setActiveWindowId(0);
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
        setActiveWindowId(0);
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
    if (error) {
        std::free(error);
    }

    if (!reply) {
        setActiveWindowId(0);
        return;
    }

    quint32 activeWindow = 0;
    if (!error
        && reply->type == XCB_ATOM_WINDOW
        && reply->format == 32
        && xcb_get_property_value_length(reply) >= static_cast<int>(sizeof(xcb_window_t))) {
        activeWindow = *static_cast<xcb_window_t *>(xcb_get_property_value(reply));
    }

    std::free(reply);
    setActiveWindowId(activeWindow);
}

} // namespace dgm
