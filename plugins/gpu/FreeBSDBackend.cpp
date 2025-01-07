/*
 * SPDX-FileCopyrightText: 2024 Henry Hu <henry.hu.sh@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "FreeBSDBackend.h"

#include <functional>
#include <string>

#include <KLocalizedString>
#include <QDebug>
#include <QRegularExpression>

#include <devinfo.h>

#include "debug.h"
#include "FreeBSDNvidiaGpu.h"

// The parent device should have name like "vgapci0", so we can extract GPU number from it.
int getGpuNumber(const QString& parent_name) {
    QRegularExpression re("(\\d+)");
    QRegularExpressionMatch match = re.match(parent_name);
    if (!match.hasMatch()) {
        return 0;
    }
    return match.captured(0).toInt();
}

FreeBSDBackend::FreeBSDBackend(QObject *parent)
    : GpuBackend(parent)
{
}

void FreeBSDBackend::start()
{
    if (devinfo_init() != 0) {
        qCWarning(KSYSTEMSTATS_GPU) << "Failed to initialize devinfo.";
        return;
    }
    devinfo_dev* root_device = devinfo_handle_to_device(DEVINFO_ROOT_DEVICE);
    devinfo_foreach_device_child(root_device, &FreeBSDBackend::findDevice, this);
}

int FreeBSDBackend::findDevice(devinfo_dev* dev, void* arg) {
    FreeBSDBackend* backend = (FreeBSDBackend*)arg;

    if (strcmp(dev->dd_drivername, "nvidia") == 0) {
        devinfo_dev* parent = devinfo_handle_to_device(dev->dd_parent);
        qCInfo(KSYSTEMSTATS_GPU) << "Found nvidia GPU:" << dev->dd_name << "(" << dev->dd_desc << ") at" << parent->dd_location;

        auto gpuName = i18nc("@title %1 is GPU number", "GPU %1", getGpuNumber(parent->dd_name));
        GpuDevice *gpu = new FreeBSDNvidiaGpu(dev->dd_name, gpuName, parent->dd_location);
        gpu->initialize();
        backend->m_devices.append(gpu);
        Q_EMIT backend->deviceAdded(gpu);
    }

    return devinfo_foreach_device_child(dev, &FreeBSDBackend::findDevice, backend);
}

void FreeBSDBackend::stop()
{
    qDeleteAll(m_devices);
    devinfo_free();
}

void FreeBSDBackend::update()
{
    for (GpuDevice* device : m_devices) {
        device->update();
    }
}

int FreeBSDBackend::deviceCount()
{
    return m_devices.count();
}

#include "moc_FreeBSDBackend.cpp"
