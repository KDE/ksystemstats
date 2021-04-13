/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CPUPLUGIN_H
#define CPUPLUGIN_H

#include <systemstats/SensorPlugin.h>

class CpuPluginPrivate;

class CpuPlugin : public KSysGuard::SensorPlugin
{
    Q_OBJECT
public:
    CpuPlugin(QObject *parent, const QVariantList &args);
    ~CpuPlugin() override;

    QString providerName() const override
    {
        return QStringLiteral("cpu");
    }
    void update() override;

private: 
   std::unique_ptr<CpuPluginPrivate> d;
};

#endif
