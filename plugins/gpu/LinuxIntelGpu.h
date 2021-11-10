/*
 * SPDX-FileCopyrightText: 2022 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "GpuDevice.h"

class QProcess;
struct udev_device;

class LinuxIntelGpu : public GpuDevice
{
    Q_OBJECT

public:
    LinuxIntelGpu(const QString &id, const QString &name, udev_device *device);
    void initialize() override;
    void makeSensors() override;

private:
    KSysGuard::SensorProperty *m_renderUsage;
    KSysGuard::SensorProperty *m_copyUsage;
    KSysGuard::SensorProperty *m_videoUsage;
    KSysGuard::SensorProperty *m_enhanceUsage;
    void readPerfData();
    QProcess *m_helperProcess;
    quint64 lastTimeStamp = 0;
    quint64 lastFrequencyCount = 0;
    std::map<KSysGuard::SensorProperty *, quint64> m_lastUsages;
};
