/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "NvidiaGpu.h"

struct udev_device;

class LinuxNvidiaGpu : public NvidiaGpu
{
    Q_OBJECT

public:

    LinuxNvidiaGpu(const QString& id, const QString& name, udev_device* device);
};
