/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxIntelGpu.h"
#include "IntelHelperLocation.h"

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

void LinuxIntelGpu::readPerfData()
{
    while (m_helperProcess->canReadLine()) {
        const QString line = m_helperProcess->readLine();
        auto parts = line.split("|");
        if (parts.size() <= 1 || parts.size() % 2 == 0) {
            continue;
        }
        quint64 timestamp = parts[0].toULong();
        quint64 timediff = timestamp - lastTimeStamp;
        double timeDiffSeconds = timediff / 1e9;
        lastTimeStamp = timestamp;

        quint64 usage = 0;
        for (int i = 1; i < parts.size(); i += 2) {
            if (parts[i] == QLatin1String("Frequency")) {
                quint64 frequencyCount = parts[i + 1].toULong();
                m_coreFrequencyProperty->setValue((frequencyCount - lastFrequencyCount) / timeDiffSeconds);
                lastFrequencyCount = frequencyCount;
            } else if (parts[i] == QLatin1String("Render") || parts[i] == QLatin1String("Copy") || parts[i] == QLatin1String("Video")
                       || parts[i] == QLatin1String("Enhance")) {
                // FIXME just adding all the "engines" is wrong (confirmed by testing, can go over 100%)
                // Either average or maybe use max, both can be potentially misleading
                usage += parts[i + 1].toULong();
            }
        }
        m_usageProperty->setValue((usage - lastUsages) * 100 / timediff);
        lastUsages = usage;
    }
}
