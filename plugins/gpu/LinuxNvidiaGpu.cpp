/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxNvidiaGpu.h"

#include <libudev.h>

namespace {
QString getPciPathFromUdev(udev_device* device) {
    return QString::fromLocal8Bit(udev_device_get_sysname(device));
}
}

LinuxNvidiaGpu::LinuxNvidiaGpu(const QString& id, const QString& name, udev_device* device)
    : NvidiaGpu(id, name, getPciPathFromUdev(device))
{
}

#include "moc_LinuxNvidiaGpu.cpp"
