/*
 * SPDX-FileCopyrightText: 2024 Henry Hu <henry.hu.sh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "GpuBackend.h"

struct devinfo_dev;

class GpuDevice;

class FreeBSDBackend : public GpuBackend
{
    Q_OBJECT

public:
    FreeBSDBackend(QObject* parent = nullptr);

    void start() override;
    void stop() override;
    void update() override;

    int deviceCount() override;

private:
    // Find GPU devices among @dev and its children.
    static int findDevice(devinfo_dev* dev, void* unused);

    QList<GpuDevice *> m_devices;
};
