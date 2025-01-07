/*
 * SPDX-FileCopyrightText: 2020 Arjen Hiemstra <ahiemstra@heimr.nl>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "NvidiaGpu.h"

#include "debug.h"

NvidiaSmiProcess *NvidiaGpu::s_smiProcess = nullptr;

constexpr quint64 mbToBytes(quint64 mb) {
    return mb * 1024 * 1024;
}

NvidiaGpu::NvidiaGpu(const QString &id, const QString &name, const QString& pci_path)
    : GpuDevice(id, name),
    m_pciPath(pci_path)
{
    if (!s_smiProcess) {
        s_smiProcess = new NvidiaSmiProcess();
    }

    connect(s_smiProcess, &NvidiaSmiProcess::dataReceived, this, &NvidiaGpu::onDataReceived);
}

NvidiaGpu::~NvidiaGpu()
{
    for (auto sensor : {m_usageProperty, m_totalVramProperty, m_usedVramProperty, m_temperatureProperty, m_coreFrequencyProperty, m_memoryFrequencyProperty}) {
        if (sensor->isSubscribed()) {
            NvidiaGpu::s_smiProcess->unref();
        }
    }
}

void NvidiaGpu::initialize()
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
                NvidiaGpu::s_smiProcess->ref();
            } else {
                NvidiaGpu::s_smiProcess->unref();
            }
        });
    }

    const auto queryResult = s_smiProcess->query();
    auto it = std::find_if(queryResult.cbegin(), queryResult.cend(), [this] (const NvidiaSmiProcess::GpuQueryResult &result) {
        return result.pciPath == m_pciPath;
    });
    if (it == queryResult.cend()) {
        qCWarning(KSYSTEMSTATS_GPU) << "Could not retrieve information for NVidia GPU" << m_pciPath;
    } else {
        m_index = it - queryResult.cbegin();
        m_nameProperty->setValue(it->name);
        m_totalVramProperty->setValue(mbToBytes(it->totalMemory));
        m_usedVramProperty->setMax(mbToBytes(it->totalMemory));
        m_coreFrequencyProperty->setMax(it->maxCoreFrequency);
        m_memoryFrequencyProperty->setMax(it->maxMemoryFrequency);
        m_temperatureProperty->setMax(it->maxTemperature);
        m_powerProperty->setMax(it->maxPower);
    }

    m_powerProperty->setUnit(KSysGuard::UnitWatt);
}

void NvidiaGpu::onDataReceived(const NvidiaSmiProcess::GpuData &data)
{
    if (data.index != m_index) {
        return;
    }

    m_usageProperty->setValue(data.usage);
    m_usedVramProperty->setValue(mbToBytes(data.memoryUsed));
    m_coreFrequencyProperty->setValue(data.coreFrequency);
    m_memoryFrequencyProperty->setValue(data.memoryFrequency);
    m_temperatureProperty->setValue(data.temperature);
    m_powerProperty->setValue(data.power);
}

#include "moc_NvidiaGpu.cpp"
