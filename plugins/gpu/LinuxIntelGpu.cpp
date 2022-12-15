/*
 * SPDX-FileCopyrightText: 2022 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxIntelGpu.h"
#include "IntelHelperLocation.h"

#include <KLocalizedString>

#include <QProcess>
#include <QStandardPaths>

LinuxIntelGpu::LinuxIntelGpu(const QString &id, const QString &name)
    : GpuDevice(id, name)
{
    m_helperProcess = new QProcess(this);
    m_helperProcess->setProgram(helperLocation);
    connect(m_helperProcess, &QProcess::readyReadStandardOutput, this, &LinuxIntelGpu::readPerfData);
    connect(m_helperProcess, &QProcess::readyReadStandardError, this, [this] {
        qCritical() << m_helperProcess->readAllStandardError();
    });
    connect(this, &GpuDevice::subscribedChanged, this, [this](bool subscribed) {
        if (subscribed) {
            m_helperProcess->start();
        } else {
            m_helperProcess->terminate();
        }
    });
}

void LinuxIntelGpu::initialize()
{
    GpuDevice::initialize();
    m_renderUsage->setName(i18nc("@title", "Render Usage"));
    m_renderUsage->setPrefix(name());
    m_renderUsage->setMin(0);
    m_renderUsage->setMax(100);
    m_renderUsage->setUnit(KSysGuard::UnitPercent);

    m_copyUsage->setName(i18nc("@title", "Copy Usage"));
    m_copyUsage->setPrefix(name());
    m_copyUsage->setMin(0);
    m_copyUsage->setMax(100);
    m_copyUsage->setUnit(KSysGuard::UnitPercent);

    m_videoUsage->setName(i18nc("@title", "Video Usage"));
    m_videoUsage->setPrefix(name());
    m_videoUsage->setMin(0);
    m_videoUsage->setMax(100);
    m_videoUsage->setUnit(KSysGuard::UnitPercent);

    m_enhanceUsage->setName(i18nc("@title", "Video Enhance Usage"));
    m_enhanceUsage->setPrefix(name());
    m_enhanceUsage->setMin(0);
    m_enhanceUsage->setMax(100);
    m_enhanceUsage->setUnit(KSysGuard::UnitPercent);
}

void LinuxIntelGpu::makeSensors()
{
    GpuDevice::makeSensors();
    m_renderUsage = new KSysGuard::SensorProperty(QStringLiteral("render usage"), QStringLiteral("render usage"), 0, this);
    m_copyUsage = new KSysGuard::SensorProperty(QStringLiteral("copy usage"), QStringLiteral("copy usage"), 0, this);
    m_videoUsage = new KSysGuard::SensorProperty(QStringLiteral("video usage"), QStringLiteral("video usage"), 0, this);
    m_enhanceUsage = new KSysGuard::SensorProperty(QStringLiteral("enhance usage"), QStringLiteral("enhance usage"), 0, this);
}

void LinuxIntelGpu::readPerfData()
{
    while (m_helperProcess->canReadLine()) {
        const QString line = m_helperProcess->readLine();
        const auto parts = line.split("|");
        if (parts.size() <= 1 || parts.size() % 2 == 0) {
            continue;
        }
        const quint64 timestamp = parts[0].toULong();
        const quint64 timediff = timestamp - lastTimeStamp;
        const double timeDiffSeconds = timediff / 1e9;
        lastTimeStamp = timestamp;
        qDebug() << parts;
        for (int i = 1; i < parts.size(); i += 2) {
            if (parts[i] == QLatin1String("Frequency")) {
                const quint64 frequencyCount = parts[i + 1].toULong();
                m_coreFrequencyProperty->setValue((frequencyCount - lastFrequencyCount) / timeDiffSeconds);
                lastFrequencyCount = frequencyCount;
            } else if (parts[i] == QLatin1String("Interrupts")) {
                // left blank for now
            } else {
                KSysGuard::SensorProperty *property;
                if (parts[i] == QLatin1String("Render")) {
                    property = m_renderUsage;
                }  else if (parts[i] == QLatin1String("Copy")) {
                    property = m_copyUsage;
                } else if (parts[i] == QLatin1String("Video")) {
                    property = m_videoUsage;
                } else if (parts[i] == QLatin1String("Enhance")) {
                    property = m_enhanceUsage;
                }
                if (!property) {
                    continue;
                }
                const auto value = parts[i + 1].toULong();
                auto &lastUsage = m_lastUsages[property];
                property->setValue((value - lastUsage * 100) / timediff);
                lastUsage = value;
            }
        }
    }
}
