// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "activewindowtracker.h"
#include "appmenuregistrar.h"
#include "menuregistry.h"

#include <applet.h>

DS_USE_NAMESPACE

class DdeFallbackActions;

class GlobalMenuApplet final : public DApplet
{
    Q_OBJECT

public:
    explicit GlobalMenuApplet(QObject *parent = nullptr);

    bool load() override;
    bool init() override;

protected:
    QObject *createProxyMeta() override;

private:
    dgm::MenuRegistry m_registry;
    dgm::AppMenuRegistrar m_registrar;
    dgm::ActiveWindowTracker *m_windowTracker = nullptr;
    DdeFallbackActions *m_fallbackActions = nullptr;
};
