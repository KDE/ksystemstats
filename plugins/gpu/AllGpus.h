/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
 */

#pragma once

#include "systemstats/SensorObject.h"

namespace KSysGuard
{
    class AggregateSensor;
}

class AllGpus : public KSysGuard::SensorObject
{
    Q_OBJECT

public:
    AllGpus(KSysGuard::SensorContainer *parent);

private:
    KSysGuard::AggregateSensor *m_usageSensor = nullptr;
    KSysGuard::AggregateSensor *m_totalVramSensor = nullptr;
    KSysGuard::AggregateSensor *m_usedVramSensor = nullptr;
};
