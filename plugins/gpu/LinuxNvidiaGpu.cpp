/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxNvidiaGpu.h"

NvidiaSmiProcess *LinuxNvidiaGpu::s_smiProcess = nullptr;

LinuxNvidiaGpu::LinuxNvidiaGpu(int index, const QString &id, const QString &name)
    : GpuDevice(id, name)
    , m_index(index)
{
    if (!s_smiProcess) {
        s_smiProcess = new NvidiaSmiProcess();
    }

    connect(s_smiProcess, &NvidiaSmiProcess::dataReceived, this, &LinuxNvidiaGpu::onDataReceived);
}

LinuxNvidiaGpu::~LinuxNvidiaGpu()
{
    for (auto sensor : {m_usageProperty, m_totalVramProperty, m_usedVramProperty, m_temperatureProperty, m_coreFrequencyProperty, m_memoryFrequencyProperty}) {
        if (sensor->isSubscribed()) {
            LinuxNvidiaGpu::s_smiProcess->unref();
        }
    }
}

void LinuxNvidiaGpu::initialize()
{
    GpuDevice::initialize();

    for (auto sensor : {m_usageProperty,
                        m_totalVramProperty,
                        m_usedVramProperty,
                        m_temperatureProperty,
                        m_coreFrequencyProperty,
                        m_memoryFrequencyProperty,
                        m_powerProperty}) {
        connect(sensor, &KSysGuard::SensorProperty::subscribedChanged, sensor, [sensor]() {
            if (sensor->isSubscribed()) {
                LinuxNvidiaGpu::s_smiProcess->ref();
            } else {
                LinuxNvidiaGpu::s_smiProcess->unref();
            }
        });
    }

    auto queryResult = s_smiProcess->query();
    if (m_index >= int(queryResult.size())) {
        qWarning() << "Could not retrieve information for NVidia GPU" << m_index;
    } else {
        auto data = queryResult.at(m_index);
        m_nameProperty->setValue(data.name);
        m_totalVramProperty->setValue(data.totalMemory);
        m_usedVramProperty->setMax(data.totalMemory);
        m_coreFrequencyProperty->setMax(data.maxCoreFrequency);
        m_memoryFrequencyProperty->setMax(data.maxMemoryFrequency);
        m_temperatureProperty->setMax(data.maxTemperature);
        m_powerProperty->setMax(data.maxPower);
    }

    m_usedVramProperty->setUnit(KSysGuard::UnitMegaByte);
    m_totalVramProperty->setUnit(KSysGuard::UnitMegaByte);
    m_powerProperty->setUnit(KSysGuard::UnitWatt);
}

void LinuxNvidiaGpu::onDataReceived(const NvidiaSmiProcess::GpuData &data)
{
    if (data.index != m_index) {
        return;
    }

    m_usageProperty->setValue(data.usage);
    m_usedVramProperty->setValue(data.memoryUsed);
    m_coreFrequencyProperty->setValue(data.coreFrequency);
    m_memoryFrequencyProperty->setValue(data.memoryFrequency);
    m_temperatureProperty->setValue(data.temperature);
    m_powerProperty->setValue(data.power);
}
