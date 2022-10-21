/*
 * SPDX-FileCopyrightText: 2022 Jesper Schmitz Mouridsen <jesper@schmitz.computer>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "SysctlBackend.h"
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <net/if_mib.h>
#include <net/if_types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/sysctl.h>
#include <systemstats/SysFsSensor.h>

SysctlNetDevice::SysctlNetDevice(const QString &id, const QString &name, int ifnumber)
    : NetworkDevice(id, name)
{
    m_sysctlName = {CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, ifnumber, IFDATA_GENERAL};
    // Even though we have no sensor, we need to have a name for the grouped text on the front page
    // of plasma-systemmonitor
    m_networkSensor->setValue(id);
    std::array<KSysGuard::SensorProperty *, 6> statisticSensors =
        {m_downloadSensor, m_downloadBitsSensor, m_totalDownloadSensor, m_uploadSensor, m_uploadBitsSensor, m_totalUploadSensor};
    auto resetStatistics = [this, statisticSensors]() {
        if (std::none_of(statisticSensors.begin(), statisticSensors.end(), [](auto property) {
                return property->isSubscribed();
            })) {
            m_totalDownloadSensor->setValue(0);
            m_totalUploadSensor->setValue(0);
            m_statisticsTimer->stop();
        } else if (!m_statisticsTimer->isActive()) {
            m_statisticsTimer->start();
        }
    };
    for (auto property : statisticSensors) {
        connect(property, &KSysGuard::SensorProperty::subscribedChanged, this, resetStatistics);
    }
    m_statisticsTimer = std::make_unique<QTimer>();
    m_statisticsTimer->setInterval(UpdateRate);
    connect(m_statisticsTimer.get(), &QTimer::timeout, this, [this]() {
        ifmibdata ifmd{};
        size_t len = sizeof(ifmd);
        if (sysctl(m_sysctlName.data(), 6, &ifmd, &len, NULL, 0) == 0) {
            const qulonglong newDownload = ifmd.ifmd_data.ifi_ibytes;
            const qulonglong previousDownload = m_totalDownloadSensor->value().toULongLong();
            const qulonglong deltaDownload = newDownload - previousDownload;
            if (previousDownload > 0) {
                m_downloadSensor->setValue(deltaDownload * 1000 / UpdateRate);
                m_downloadBitsSensor->setValue((deltaDownload * 1000 / UpdateRate) * 8);
            }
            m_totalDownloadSensor->setValue(newDownload);

            const qulonglong newUpload = ifmd.ifmd_data.ifi_obytes;
            const qulonglong previousUpload = m_totalUploadSensor->value().toULongLong();
            const qulonglong deltaUpload = newUpload - previousUpload;
            if (previousUpload > 0) {
                m_uploadSensor->setValue(deltaUpload * 1000 / UpdateRate);
                m_uploadBitsSensor->setValue((deltaUpload * 1000 / UpdateRate) * 8);
            }
            m_totalUploadSensor->setValue(newUpload);
        }
    });
}
void SysctlNetDevice::update()
{
    char addr_buf[NI_MAXHOST];
    QStringList ipv4_addrs;
    QStringList ipv6_addrs;
    struct sockaddr_in *sin;

    ipv4_addrs.clear();
    ipv6_addrs.clear();
    ifaddrs *ifap;
    bzero(&ifap, sizeof(ifap));
    for (getifaddrs(&ifap); ifap != nullptr; ifap = ifap->ifa_next) {
        if (name() == QString::fromLatin1(ifap->ifa_name)) {
            sin = reinterpret_cast<struct sockaddr_in *>(ifap->ifa_addr);
            if (sin == NULL)
                return;

            getnameinfo(ifap->ifa_addr, sin->sin_len, addr_buf, sizeof(addr_buf), NULL, 0, NI_NUMERICHOST);

            if (sin->sin_family == AF_INET) {
                ipv4_addrs << QString::fromStdString(addr_buf);
            }
            if (sin->sin_family == AF_INET6) {
                ipv6_addrs << QString::fromStdString(addr_buf);
            }

            m_ipv4Sensor->setValue(ipv4_addrs.join("\n"));
            m_ipv6Sensor->setValue(ipv6_addrs.join("\n"));
        }
    }
    freeifaddrs(ifap);
}

SysctlNetDevice::~SysctlNetDevice()
{
}
SysctlBackend::SysctlBackend(QObject *parent)
    : NetworkBackend(parent)
{
}

SysctlBackend::~SysctlBackend()
{
    qDeleteAll(m_devices);
}

bool SysctlBackend::isSupported()
{
    return true;
}

void SysctlBackend::start()
{
    int count, i;
    size_t len;
    int ifcnt_name[] = {CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_SYSTEM, IFMIB_IFCOUNT};
    len = sizeof(int);
    if (sysctl(ifcnt_name, 5, &count, &len, NULL, 0) < 0) {
        return;
    }

    for (i = 1; i <= count; i++) {
        struct ifmibdata ifmd {
        };
        size_t len = sizeof(ifmd);
        int sname[6] = {CTL_NET, PF_LINK, NETLINK_GENERIC, IFMIB_IFDATA, i, IFDATA_GENERAL};

        if (sysctl(sname, 6, &ifmd, &len, NULL, 0) < 0) {
            continue;
        }

        if ((ifmd.ifmd_data.ifi_type == IFT_ETHER) || (ifmd.ifmd_data.ifi_type == IFT_IEEE80211)) {
            auto device = new SysctlNetDevice(QString::fromStdString(ifmd.ifmd_name), QString::fromStdString(ifmd.ifmd_name), i);

            m_devices.insert(QByteArray(ifmd.ifmd_name), device);
            Q_EMIT deviceAdded(device);
        }
    }
}

void SysctlBackend::stop()
{
}

void SysctlBackend::update()
{
    QHashIterator<QByteArray, SysctlNetDevice *> iter(m_devices);
    while (iter.hasNext()) {
        iter.next();
        iter.value()->update();
    }
}
