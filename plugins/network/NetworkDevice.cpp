/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "NetworkDevice.h"

#include <KLocalizedString>

NetworkDevice::NetworkDevice(const QString &id, const QString &name)
    : SensorObject(id, name, nullptr)
{
    m_networkSensor = new KSysGuard::SensorProperty(QStringLiteral("network"), i18nc("@title", "Network Name"), this);
    m_networkSensor->setShortName(i18nc("@title Short of Network Name", "Name"));
    m_networkSensor->setPrefix(name);

    m_signalSensor = new KSysGuard::SensorProperty(QStringLiteral("signal"), i18nc("@title", "Signal Strength"), this);
    m_signalSensor->setShortName(i18nc("@title Short of Signal Strength", "Signal"));
    m_signalSensor->setUnit(KSysGuard::UnitPercent);
    m_signalSensor->setMin(0);
    m_signalSensor->setMax(100);
    m_signalSensor->setPrefix(name);

    m_ipv4Sensor = new KSysGuard::SensorProperty(QStringLiteral("ipv4address"), i18nc("@title", "IPv4 Address"), this);
    m_ipv4Sensor->setShortName(i18nc("@title Short of IPv4 Address", "IPv4"));
    m_ipv4Sensor->setPrefix(name);

    m_ipv6Sensor = new KSysGuard::SensorProperty(QStringLiteral("ipv6address"), i18nc("@title", "IPv6 Address"), this);
    m_ipv6Sensor->setShortName(i18nc("@title Short of IPv6 Address", "IPv6"));
    m_ipv6Sensor->setPrefix(name);

    m_downloadSensor = new KSysGuard::SensorProperty(QStringLiteral("download"), i18nc("@title", "Download Rate"), this);
    m_downloadSensor->setShortName(i18nc("@title Short for Download Rate", "Download"));
    m_downloadSensor->setUnit(KSysGuard::UnitByteRate);
    m_downloadSensor->setPrefix(name);

    m_uploadSensor = new KSysGuard::SensorProperty(QStringLiteral("upload"), i18nc("@title", "Upload Rate"), this);
    m_uploadSensor->setShortName(i18nc("@title Short for Upload Rate", "Upload"));
    m_uploadSensor->setUnit(KSysGuard::UnitByteRate);
    m_uploadSensor->setPrefix(name);

    m_downloadBitsSensor = new KSysGuard::SensorProperty(QStringLiteral("downloadBits"), i18nc("@title", "Download Rate"), this);
    m_downloadBitsSensor->setShortName(i18nc("@title Short for Download Rate", "Download"));
    m_downloadBitsSensor->setUnit(KSysGuard::UnitBitRate);
    m_downloadBitsSensor->setPrefix(name);

    m_uploadBitsSensor = new KSysGuard::SensorProperty(QStringLiteral("uploadBits"), i18nc("@title", "Upload Rate"), this);
    m_uploadBitsSensor->setShortName(i18nc("@title Short for Upload Rate", "Upload"));
    m_uploadBitsSensor->setUnit(KSysGuard::UnitBitRate);
    m_uploadBitsSensor->setPrefix(name);

    m_totalDownloadSensor = new KSysGuard::SensorProperty(QStringLiteral("totalDownload"), i18nc("@title", "Total Downloaded"), this);
    m_totalDownloadSensor->setShortName(i18nc("@title Short for Total Downloaded", "Downloaded"));
    m_totalDownloadSensor->setUnit(KSysGuard::UnitByte);
    m_totalDownloadSensor->setPrefix(name);

    m_totalUploadSensor = new KSysGuard::SensorProperty(QStringLiteral("totalUpload"), i18nc("@title", "Total Uploaded"), this);
    m_totalUploadSensor->setShortName(i18nc("@title Short for Total Uploaded", "Uploaded"));
    m_totalUploadSensor->setUnit(KSysGuard::UnitByte);
    m_totalDownloadSensor->setPrefix(name);
}
