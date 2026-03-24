/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * 
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxBackend.h"

#include <KLocalizedString>

#include <libudev.h>

#include "LinuxAmdGpu.h"
#include "LinuxIntelGpu.h"
#include "LinuxNvidiaGpu.h"
#include "debug.h"

// Vendor ID strings, as used in sysfs
static const char *amdVendor = "0x1002";
static const char *intelVendor = "0x8086";
static const char *nvidiaVendor = "0x10de";
// PCI Device class strings
static const char *VGAController = "0x030000";
static const char *threeDController = "0x030200";

LinuxBackend::LinuxBackend(QObject *parent)
    : GpuBackend(parent)
{
}

void LinuxBackend::start()
{
    if (!m_udev) {
        m_udev = udev_new();
    }

    auto enumerate = udev_enumerate_new(m_udev);

    udev_enumerate_add_match_subsystem(enumerate, "pci");
    udev_enumerate_add_match_sysattr(enumerate, "class", VGAController);
    udev_enumerate_add_match_sysattr(enumerate, "class", threeDController);
    udev_enumerate_scan_devices(enumerate);

    auto devices = udev_enumerate_get_list_entry(enumerate);
    int gpuNumber = 0;
    for (auto entry = devices; entry; entry = udev_list_entry_get_next(entry)) {
        auto path = udev_list_entry_get_name(entry);
        auto pciDevice = udev_device_new_from_syspath(m_udev, path);

        auto vendor = QByteArray(udev_device_get_sysattr_value(pciDevice, "vendor"));
        auto gpuId = QStringLiteral("gpu%1").arg(gpuNumber);
        auto gpuName = i18nc("@title %1 is GPU number", "GPU %1", gpuNumber + 1);

        GpuDevice *gpu = nullptr;
        if (vendor == amdVendor) {
            gpu = new LinuxAmdGpu{gpuId, gpuName, pciDevice};
        } else if (vendor == nvidiaVendor) {
            gpu = new LinuxNvidiaGpu{gpuId, gpuName, pciDevice};
        } else if (vendor == intelVendor) {
            gpu = new LinuxIntelGpu{gpuId, gpuName, pciDevice};
        } else {
            qCDebug(KSYSTEMSTATS_GPU) << "Found unsupported GPU:" << path;
            udev_device_unref(pciDevice);
            continue;
        }

        gpu->initialize();
        m_devices.append(gpu);
        Q_EMIT deviceAdded(gpu);
        gpuNumber++;

        udev_device_unref(pciDevice);
    }

    udev_enumerate_unref(enumerate);
}

void LinuxBackend::stop()
{
    qDeleteAll(m_devices);
    udev_unref(m_udev);
}

void LinuxBackend::update()
{
    for (auto device : std::as_const(m_devices)) {
        device->update();
    }
}

int LinuxBackend::deviceCount()
{
    return m_devices.count();
}

#include "moc_LinuxBackend.cpp"
