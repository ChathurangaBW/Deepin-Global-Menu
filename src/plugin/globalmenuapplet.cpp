// SPDX-License-Identifier: GPL-3.0-or-later

#include "globalmenuapplet.h"

#include "globalmenucontroller.h"

#include <pluginfactory.h>
#include <QDebug>

GlobalMenuApplet::GlobalMenuApplet(QObject *parent)
    : DApplet(parent)
    , m_registry(this)
    , m_registrar(&m_registry, this)
{
}

bool GlobalMenuApplet::load()
{
    if (!m_registrar.start()) {
        qWarning() << "Deepin Global Menu: registrar did not start; the applet will load in diagnostic mode";
    }

    return DApplet::load();
}

bool GlobalMenuApplet::init()
{
    return DApplet::init();
}

QObject *GlobalMenuApplet::createProxyMeta()
{
    return new dgm::GlobalMenuController(&m_registry, this);
}

D_APPLET_CLASS(GlobalMenuApplet)

#include "globalmenuapplet.moc"
