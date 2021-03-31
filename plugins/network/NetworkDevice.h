/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
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
    KSysGuard::SensorProperty *m_networkSensor;
    KSysGuard::SensorProperty *m_signalSensor;
    KSysGuard::SensorProperty *m_ipv4Sensor;
    KSysGuard::SensorProperty *m_ipv6Sensor;
    KSysGuard::SensorProperty *m_downloadSensor;
    KSysGuard::SensorProperty *m_uploadSensor;
    KSysGuard::SensorProperty *m_totalDownloadSensor;
    KSysGuard::SensorProperty *m_totalUploadSensor;
};
