/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "GpuDevice.h"

class QProcess;

class LinuxIntelGpu : public GpuDevice
{
    Q_OBJECT

public:
    LinuxIntelGpu(const QString &id, const QString &name);

private:
    void readPerfData();
    QProcess *m_helperProcess;
    quint64 lastTimeStamp;
    quint64 lastUsages;
    quint64 lastFrequencyCount;
};
