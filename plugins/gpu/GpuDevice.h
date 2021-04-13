/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include "systemstats/SensorObject.h"

class GpuDevice : public KSysGuard::SensorObject
{
    Q_OBJECT

public:
    GpuDevice(const QString& id, const QString& name);
    ~GpuDevice() override = default;

    virtual void initialize();
    virtual void update();

protected:
    virtual void makeSensors();

    KSysGuard::SensorProperty *m_nameProperty;
    KSysGuard::SensorProperty *m_usageProperty;
    KSysGuard::SensorProperty *m_totalVramProperty;
    KSysGuard::SensorProperty *m_usedVramProperty;
    KSysGuard::SensorProperty *m_temperatureProperty;
    KSysGuard::SensorProperty *m_coreFrequencyProperty;
    KSysGuard::SensorProperty *m_memoryFrequencyProperty;
};
