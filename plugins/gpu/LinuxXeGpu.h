/*
 * SPDX-FileCopyrightText: 2025 Hunter Hardy <thehunterhardy@icloud.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "GpuDevice.h"

class QProcess;
struct udev_device;

namespace KSysGuard
{
    class SysFsSensor;
}

class LinuxXeGpu : public GpuDevice
{
    Q_OBJECT

public:
    LinuxXeGpu(const QString &id, const QString &name, udev_device *device);
    ~LinuxXeGpu() override;

    void initialize() override;
    void update() override;

protected:
    void makeSensors() override;

private:
    void readPerfData();
    void discoverHwmonSensors();
    void queryVram();

    udev_device *m_device;
    QProcess *m_helperProcess;
    int m_drmFd = -1;

    QList<KSysGuard::SysFsSensor *> m_hwmonSensors;
    QList<KSysGuard::SysFsSensor *> m_fanSensors;
    KSysGuard::SysFsSensor *m_vramTempSensor = nullptr;
    KSysGuard::SensorProperty *m_videoUsage = nullptr;
    QString m_hwmonPath;
};
