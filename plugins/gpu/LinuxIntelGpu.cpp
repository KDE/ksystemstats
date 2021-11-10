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

#include <libudev.h>

LinuxIntelGpu::LinuxIntelGpu(const QString &id, const QString &name, udev_device *device)
    : GpuDevice(id, name)
{
    m_helperProcess = new QProcess(this);
    m_helperProcess->setProgram(helperLocation);
    m_helperProcess->setArguments({udev_device_get_sysname(device)});
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

    m_videoUsage->setName(i18nc("@title", "Video Usage"));
    m_videoUsage->setPrefix(name());
    m_videoUsage->setMin(0);
    m_videoUsage->setMax(100);
    m_videoUsage->setUnit(KSysGuard::UnitPercent);
}

void LinuxIntelGpu::makeSensors()
{
    GpuDevice::makeSensors();

    m_videoUsage = new KSysGuard::SensorProperty(QStringLiteral("video"), QStringLiteral("video"), 0, this);
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
        const double timeDiffSeconds = timediff / static_cast<double>(std::chrono::nanoseconds(std::chrono::seconds(1)).count());

        lastTimeStamp = timestamp;
        for (int i = 1; i < parts.size(); i += 2) {
            if (parts[i] == QLatin1String("Frequency")) {
                const quint64 frequencyCount = parts[i + 1].toULong();
                m_coreFrequencyProperty->setValue((frequencyCount - lastFrequencyCount) / timeDiffSeconds);
                lastFrequencyCount = frequencyCount;
            } else {
                KSysGuard::SensorProperty *property = nullptr;
                if (parts[i] == QLatin1String("Render")) {
                    property = m_usageProperty;
                } else if (parts[i] == QLatin1String("Video")) {
                    property = m_videoUsage;
                }
                if (!property) {
                    continue;
                }
                const auto value = parts[i + 1].toULong();
                auto &lastUsage = m_lastUsages[property];
                property->setValue((value - lastUsage) * 100.0 / timediff);
                lastUsage = value;
            }
        }
    }
}
