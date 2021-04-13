/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef DISKS_H
#define DISKS_H

#include <QObject>
#include <QElapsedTimer>

#include "systemstats/SensorPlugin.h"

namespace Solid {
    class Device;
    class StorageVolume;
}

class VolumeObject;

class DisksPlugin : public KSysGuard::SensorPlugin

{
    Q_OBJECT
public:
    DisksPlugin(QObject *parent, const QVariantList &args);
    QString providerName() const override
    {
        return QStringLiteral("disks");
    }
    ~DisksPlugin() override;

    void update() override;


private:
    void addDevice(const Solid::Device &device);
    void addAggregateSensors();

    QHash<QString, VolumeObject*> m_volumesByDevice;
    QElapsedTimer m_elapsedTimer;
};

#endif
