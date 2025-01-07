/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "GpuDevice.h"
#include "NvidiaSmiProcess.h"

class NvidiaGpu : public GpuDevice
{
    Q_OBJECT

public:
    NvidiaGpu(const QString& id, const QString &name, const QString& pci_path);
    ~NvidiaGpu() override;

    void initialize() override;

private:
    void onDataReceived(const NvidiaSmiProcess::GpuData &data);

    int m_index = -1;

    QString m_pciPath;

    static NvidiaSmiProcess *s_smiProcess;
};
