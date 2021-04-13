/*
    SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#pragma once

#include <systemstats/SensorPlugin.h>

class OSInfoPrivate;

class OSInfoPlugin : public KSysGuard::SensorPlugin
{
    Q_OBJECT
public:
    OSInfoPlugin(QObject *parent, const QVariantList &args);
    ~OSInfoPlugin() override;

    QString providerName() const override
    {
        return QStringLiteral("osinfo");
    }

private:
    std::unique_ptr<OSInfoPrivate> d;
};
