/*
 * SPDX-FileCopyrightText: 2022 Jesper Schmitz Mouridsen <jesper@schmitz.computer>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "NetworkBackend.h"
#include "NetworkDevice.h"
#include <array>
#include <QElapsedTimer>
#include <QTimer>

static const int UpdateRate = 500;
class SysctlNetDevice : public NetworkDevice
{
    Q_OBJECT

public:
    SysctlNetDevice(const QString &id, const QString &name,int ifnumber);
    ~SysctlNetDevice() override;
    std::array<int,6> m_sysctlName;
    void update();
    bool isConnected() const;
    std::unique_ptr<QTimer> m_statisticsTimer;
    QList<struct sockaddr *> m_ifaddrs;
};

class SysctlBackend : public NetworkBackend
{
public:
    SysctlBackend(QObject *parent);
    ~SysctlBackend() override;
    bool isSupported() override;
    void start() override;
    void stop() override;
    void update() override;

private:
    QHash<QByteArray, SysctlNetDevice *> m_devices;
};
