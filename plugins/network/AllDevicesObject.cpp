/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.1-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "AllDevicesObject.h"

#include <KLocalizedString>

#include <systemstats/AggregateSensor.h>

#include "NetworkDevice.h"

AllDevicesObject::AllDevicesObject(KSysGuard::SensorContainer *parent)
    : SensorObject(QStringLiteral("all"), i18nc("@title", "All Network Devices"), parent)
{
    m_downloadSensor = new KSysGuard::AggregateSensor(this, QStringLiteral("download"), i18nc("@title", "Download Rate"));
    m_downloadSensor->setShortName(i18nc("@title Short for Download Rate", "Download"));
    m_downloadSensor->setUnit(KSysGuard::UnitByteRate);
    m_downloadSensor->setMatchSensors(QRegularExpression{"^(?!all).*$"}, QStringLiteral("download"));

    m_uploadSensor = new KSysGuard::AggregateSensor(this, QStringLiteral("upload"), i18nc("@title", "Upload Rate"));
    m_uploadSensor->setShortName(i18nc("@title Short for Upload Rate", "Upload"));
    m_uploadSensor->setUnit(KSysGuard::UnitByteRate);
    m_uploadSensor->setMatchSensors(QRegularExpression{"^(?!all).*$"}, QStringLiteral("upload"));

    m_totalDownloadSensor = new KSysGuard::AggregateSensor(this, QStringLiteral("totalDownload"), i18nc("@title", "Total Downloaded"));
    m_totalDownloadSensor->setShortName(i18nc("@title Short for Total Downloaded", "Downloaded"));
    m_totalDownloadSensor->setUnit(KSysGuard::UnitByte);
    m_totalDownloadSensor->setMatchSensors(QRegularExpression{"^(?!all).*$"}, QStringLiteral("totalDownload"));

    m_totalUploadSensor = new KSysGuard::AggregateSensor(this, QStringLiteral("totalUpload"), i18nc("@title", "Total Uploaded"));
    m_totalUploadSensor->setShortName(i18nc("@title Short for Total Uploaded", "Uploaded"));
    m_totalUploadSensor->setUnit(KSysGuard::UnitByte);
    m_totalUploadSensor->setMatchSensors(QRegularExpression{"^(?!all).*$"}, QStringLiteral("totalUpload"));
}
