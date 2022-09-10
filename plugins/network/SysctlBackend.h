/*
 * SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "NetworkBackend.h"
#include "NetworkDevice.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <net/if_media.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>
#include <ifaddrs.h>
#include <sys/sysctl.h>
#include <net/if_mib.h>
#include <net/if_types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>
#include <netdb.h>
#include <QElapsedTimer>
#include <QTimer>
static const int UpdateRate = 500;
class SysctlNetDevice : public NetworkDevice
{
    Q_OBJECT

public:
    SysctlNetDevice(const QString &id, const QString &name);
    ~SysctlNetDevice() override;
    int m_sysctl_name[6] = { CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, 0, IFDATA_GENERAL };
    void update();
    bool isConnected() const;
    std::unique_ptr<QTimer> m_statisticsTimer;
    QList<struct sockaddr*>  m_ifaddrs;
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
    QElapsedTimer m_updateTimer;

};
