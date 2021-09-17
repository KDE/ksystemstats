/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "systemstats/SensorObject.h"

class NetworkDevice : public KSysGuard::SensorObject
{
    Q_OBJECT

public:
    NetworkDevice(const QString& id, const QString& name);
    ~NetworkDevice() override = default;

protected:
    KSysGuard::SensorProperty *m_networkSensor = nullptr;
    KSysGuard::SensorProperty *m_signalSensor = nullptr;
    KSysGuard::SensorProperty *m_ipv4Sensor = nullptr;
    KSysGuard::SensorProperty *m_ipv6Sensor = nullptr;
    KSysGuard::SensorProperty *m_downloadSensor = nullptr;
    KSysGuard::SensorProperty *m_uploadSensor = nullptr;
    KSysGuard::SensorProperty *m_totalDownloadSensor = nullptr;
    KSysGuard::SensorProperty *m_totalUploadSensor = nullptr;
};
