/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxAmdGpu.h"

#include <libudev.h>
#include <linux/pci.h>
#include <sensors/sensors.h>

#include <QDir>

#include <systemstats/SysFsSensor.h>
#include <systemstats/SensorsFeatureSensor.h>

int ppTableGetMax(const QByteArray &table)
{
    const auto lines = table.split('\n');
    auto line = lines.last();
    return std::atoi(line.mid(line.indexOf(':') + 1).data());
}

int ppTableGetCurrent(const QByteArray &table)
{
    const auto lines = table.split('\n');

    int current = 0;
    for (auto line : lines) {
        if (!line.contains('*')) {
            continue;
        }

        current = std::atoi(line.mid(line.indexOf(':') + 1));
    }

    return current;
}

LinuxAmdGpu::LinuxAmdGpu(const QString& id, const QString& name, udev_device *device)
    : GpuDevice(id, name)
    , m_device(device)
{
    udev_device_ref(m_device);
}

LinuxAmdGpu::~LinuxAmdGpu()
{
    udev_device_unref(m_device);
}

void LinuxAmdGpu::initialize()
{
    GpuDevice::initialize();

    m_nameProperty->setValue(QString::fromLocal8Bit(udev_device_get_property_value(m_device, "ID_MODEL_FROM_DATABASE")));

    auto result = udev_device_get_sysattr_value(m_device, "mem_info_vram_total");
    if (result) {
        m_totalVramProperty->setValue(std::atoll(result));
    }

    m_coreFrequencyProperty->setMax(ppTableGetMax(udev_device_get_sysattr_value(m_device, "pp_dpm_sclk")));
    m_memoryFrequencyProperty->setMax(ppTableGetMax(udev_device_get_sysattr_value(m_device, "pp_dpm_mclk")));
}

void LinuxAmdGpu::update()
{
    for (auto sensor : m_sysFsSensors) {
        sensor->update();
    }
    m_temperatureProperty->update();
}

void LinuxAmdGpu::makeSensors()
{
    auto devicePath = QString::fromLocal8Bit(udev_device_get_syspath(m_device));

    m_nameProperty = new KSysGuard::SensorProperty(QStringLiteral("name"), this);
    m_totalVramProperty = new KSysGuard::SensorProperty(QStringLiteral("totalVram"),  this);

    auto sensor = new KSysGuard::SysFsSensor(QStringLiteral("usage"), devicePath % QStringLiteral("/gpu_busy_percent"), this);
    m_usageProperty = sensor;
    m_sysFsSensors << sensor;

    sensor = new KSysGuard::SysFsSensor(QStringLiteral("usedVram"), devicePath % QStringLiteral("/mem_info_vram_used"), this);
    m_usedVramProperty = sensor;
    m_sysFsSensors << sensor;

    sensor = new KSysGuard::SysFsSensor(QStringLiteral("coreFrequency"), devicePath % QStringLiteral("/pp_dpm_sclk"), this);
    sensor->setConvertFunction([](const QByteArray &input) {
        return ppTableGetCurrent(input);
    });
    m_coreFrequencyProperty = sensor;
    m_sysFsSensors << sensor;

    sensor = new KSysGuard::SysFsSensor(QStringLiteral("memoryFrequency"), devicePath % QStringLiteral("/pp_dpm_mclk"), this);
    sensor->setConvertFunction([](const QByteArray &input) {
        return ppTableGetCurrent(input);
    });
    m_memoryFrequencyProperty = sensor;
    m_sysFsSensors << sensor;

    sensors_chip_name match;
    sensors_parse_chip_name("amdgpu-*", &match);
    int number = 0;
    const sensors_chip_name *chip;
    while ((chip = sensors_get_detected_chips(&match, &number))) {
        int domain, bus, device, function;
        if (std::sscanf(udev_device_get_sysname(m_device), "%d:%d:%d.%d", &domain, &bus, &device, &function) == 4 &&
            ((domain << 16) + (bus << 8) + PCI_DEVFN(device, function)) == chip->addr) {
            break;
        }
    }
    number = 0;
    const sensors_feature * feature;
    while (chip && (feature = sensors_get_features(chip, &number))) {
        if (feature->type == SENSORS_FEATURE_TEMP && qstrcmp(feature->name, "temp1") == 0) {
            m_temperatureProperty = KSysGuard::makeSensorsFeatureSensor(QStringLiteral("temperature"), chip, feature, this);
        }
    }
    if (!m_temperatureProperty) {
        m_temperatureProperty = new KSysGuard::SensorProperty(QStringLiteral("temperature"), this);
    }
}
