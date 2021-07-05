/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "GpuDevice.h"

struct udev_device;

namespace KSysGuard
{
    class SysFsSensor;
}

class LinuxAmdGpu : public GpuDevice
{
    Q_OBJECT

public:
    LinuxAmdGpu(const QString& id, const QString& name, udev_device *device);
    ~LinuxAmdGpu() override;

    void initialize() override;
    void update() override;

protected:
    void makeSensors() override;

private:
    udev_device *m_device;
    QVector<KSysGuard::SysFsSensor*> m_sysFsSensors;
};
