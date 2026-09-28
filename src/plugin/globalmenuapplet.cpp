// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalmenuapplet.h"

#include "ddeactivewindowtracker.h"
#include "ddefallbackactions.h"
#include "globalmenucontroller.h"
#include "shortcutexecutor.h"

#include <pluginfactory.h>

#include <QDebug>
#include <QGuiApplication>

GlobalMenuApplet::GlobalMenuApplet(QObject *parent)
    : DApplet(parent)
    , m_registry(this)
    , m_registrar(&m_registry, this)
    , m_fallbackActions(new DdeFallbackActions(this))
    , m_shortcutExecutor(new ShortcutExecutor(this))
{
    if (QGuiApplication::platformName() == QStringLiteral("wayland")) {
        m_windowTracker = new DdeActiveWindowTracker(this);
    } else {
        m_windowTracker = dgm::createActiveWindowTracker(this);
    }
}

bool GlobalMenuApplet::load()
{
    if (!m_registrar.start()) {
        qWarning() << "Deepin Global Menu: registrar did not start; another registrar may already own the service";
    }

    if (!m_windowTracker || !m_windowTracker->start()) {
        qWarning() << "Deepin Global Menu: automatic active-window tracking is unavailable in this session";
    }

    return DApplet::load();
}

bool GlobalMenuApplet::init()
{
    return DApplet::init();
}

QObject *GlobalMenuApplet::createProxyMeta()
{
    auto *controller = new dgm::GlobalMenuController(&m_registry, this);
    controller->setActiveWindowTracker(m_windowTracker);
    controller->setShortcutActionsAvailable(m_shortcutExecutor->available());

    connect(controller, &dgm::GlobalMenuController::fallbackActionRequested,
            m_fallbackActions, &DdeFallbackActions::execute);
    connect(controller, &dgm::GlobalMenuController::shortcutRequested,
            m_shortcutExecutor, &ShortcutExecutor::send);
    connect(m_shortcutExecutor, &ShortcutExecutor::availabilityChanged,
            controller, [controller, this] {
                controller->setShortcutActionsAvailable(
                    m_shortcutExecutor->available());
            });

    return controller;
}

D_APPLET_CLASS(GlobalMenuApplet)

#include "globalmenuapplet.moc"
