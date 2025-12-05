/*
 * SPDX-FileCopyrightText: 2025 Hunter Hardy <thehunterhardy@icloud.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "LinuxXeGpu.h"
#include "XeHelperLocation.h"

#include <KLocalizedString>

#include <QDir>
#include <QFile>
#include <QProcess>

#include <fcntl.h>
#include <libudev.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <drm/xe_drm.h>

#include <systemstats/SysFsSensor.h>

LinuxXeGpu::LinuxXeGpu(const QString &id, const QString &name, udev_device *device)
    : GpuDevice(id, name)
    , m_device(device)
{
    udev_device_ref(m_device);

    // Open DRM device for VRAM queries (doesn't require special privileges)
    const char *sysnum = udev_device_get_sysnum(m_device);
    if (sysnum) {
        QString cardPath = QStringLiteral("/dev/dri/card") + QString::fromLatin1(sysnum);
        m_drmFd = open(cardPath.toLatin1().constData(), O_RDONLY);
    }

    m_helperProcess = new QProcess(this);
    m_helperProcess->setProgram(xeHelperLocation);

    auto pciDevice = udev_device_get_parent(m_device);
    if (pciDevice) {
        const char *pciSlot = udev_device_get_sysattr_value(pciDevice, "address");
        if (!pciSlot) {
            pciSlot = udev_device_get_sysname(pciDevice);
        }
        if (pciSlot) {
            m_helperProcess->setArguments({QString::fromLatin1(pciSlot)});
        }
    }

    connect(m_helperProcess, &QProcess::readyReadStandardOutput, this, &LinuxXeGpu::readPerfData);
    connect(m_helperProcess, &QProcess::readyReadStandardError, this, [this] {
        qCritical() << m_helperProcess->readAllStandardError();
    });
    connect(this, &GpuDevice::subscribedChanged, this, [this](bool subscribed) {
        if (subscribed) {
            m_helperProcess->start();
        } else {
            m_helperProcess->terminate();
        }
    });
}

LinuxXeGpu::~LinuxXeGpu()
{
    if (m_drmFd >= 0) {
        close(m_drmFd);
    }
    udev_device_unref(m_device);
}

void LinuxXeGpu::initialize()
{
    GpuDevice::initialize();

    const char *modelName = udev_device_get_property_value(m_device, "ID_MODEL_FROM_DATABASE");
    if (modelName) {
        m_nameProperty->setValue(QString::fromLocal8Bit(modelName));
    }

    if (m_videoUsage) {
        m_videoUsage->setName(i18nc("@title", "Video Usage"));
        m_videoUsage->setPrefix(name());
        m_videoUsage->setMin(0);
        m_videoUsage->setMax(100);
        m_videoUsage->setUnit(KSysGuard::UnitPercent);
    }

    if (m_copyUsage) {
        m_copyUsage->setName(i18nc("@title", "Copy Usage"));
        m_copyUsage->setPrefix(name());
        m_copyUsage->setMin(0);
        m_copyUsage->setMax(100);
        m_copyUsage->setUnit(KSysGuard::UnitPercent);
    }

    if (m_enhanceUsage) {
        m_enhanceUsage->setName(i18nc("@title", "Enhance Usage"));
        m_enhanceUsage->setPrefix(name());
        m_enhanceUsage->setMin(0);
        m_enhanceUsage->setMax(100);
        m_enhanceUsage->setUnit(KSysGuard::UnitPercent);
    }

    for (auto sensor : std::as_const(m_hwmonSensors)) {
        sensor->setPrefix(name());
    }
}

void LinuxXeGpu::update()
{
    for (auto sensor : std::as_const(m_hwmonSensors)) {
        sensor->update();
    }
    queryVram();
}

void LinuxXeGpu::makeSensors()
{
    GpuDevice::makeSensors();

    m_videoUsage = new KSysGuard::SensorProperty(QStringLiteral("video"), QStringLiteral("video"), 0, this);
    m_copyUsage = new KSysGuard::SensorProperty(QStringLiteral("copy"), QStringLiteral("copy"), 0, this);
    m_enhanceUsage = new KSysGuard::SensorProperty(QStringLiteral("enhance"), QStringLiteral("enhance"), 0, this);

    discoverHwmonSensors();
}

void LinuxXeGpu::discoverHwmonSensors()
{
    auto pciDevice = udev_device_get_parent(m_device);
    if (!pciDevice) {
        return;
    }

    QString pciPath = QString::fromLocal8Bit(udev_device_get_syspath(pciDevice));
    QDir hwmonDir(pciPath + QStringLiteral("/hwmon"));

    if (!hwmonDir.exists()) {
        return;
    }

    QStringList hwmonDirs = hwmonDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    if (hwmonDirs.isEmpty()) {
        return;
    }

    m_hwmonPath = hwmonDir.absoluteFilePath(hwmonDirs.first());

    QFile nameFile(m_hwmonPath + QStringLiteral("/name"));
    if (nameFile.open(QIODevice::ReadOnly)) {
        QString hwmonName = QString::fromLatin1(nameFile.readAll().trimmed());
        if (hwmonName != QStringLiteral("xe")) {
            return;
        }
    }

    QFile tempLabelFile(m_hwmonPath + QStringLiteral("/temp2_label"));
    if (tempLabelFile.exists()) {
        auto sensor = new KSysGuard::SysFsSensor(QStringLiteral("temperature"),
                                                  m_hwmonPath + QStringLiteral("/temp2_input"),
                                                  0, this);
        sensor->setConvertFunction([](const QByteArray &input) {
            return input.toLongLong() / 1000.0;
        });
        m_temperatureProperty = sensor;
        m_hwmonSensors << sensor;
    }
}

void LinuxXeGpu::queryVram()
{
    if (m_drmFd < 0) {
        return;
    }

    drm_xe_device_query query{};
    query.query = DRM_XE_DEVICE_QUERY_MEM_REGIONS;
    query.size = 0;
    query.data = 0;

    if (ioctl(m_drmFd, DRM_IOCTL_XE_DEVICE_QUERY, &query) != 0 || query.size == 0) {
        return;
    }

    QByteArray buffer(query.size, 0);
    query.data = reinterpret_cast<std::uint64_t>(buffer.data());

    if (ioctl(m_drmFd, DRM_IOCTL_XE_DEVICE_QUERY, &query) != 0) {
        return;
    }

    auto *regions = reinterpret_cast<drm_xe_query_mem_regions *>(buffer.data());
    std::uint64_t totalVram = 0;
    std::uint64_t usedVram = 0;

    for (std::uint32_t i = 0; i < regions->num_mem_regions; ++i) {
        if (regions->mem_regions[i].mem_class == DRM_XE_MEM_REGION_CLASS_VRAM) {
            totalVram += regions->mem_regions[i].total_size;
            usedVram += regions->mem_regions[i].used;
        }
    }

    if (totalVram > 0) {
        m_totalVramProperty->setValue(static_cast<qulonglong>(totalVram));
        m_usedVramProperty->setValue(static_cast<qulonglong>(usedVram));
    }
}

void LinuxXeGpu::readPerfData()
{
    while (m_helperProcess->canReadLine()) {
        const QString line = QString::fromLatin1(m_helperProcess->readLine());
        const auto parts = line.split(QLatin1Char('|'));

        if (parts.size() <= 1 || parts.size() % 2 == 0) {
            continue;
        }

        for (int i = 1; i < parts.size(); i += 2) {
            const QString &label = parts[i];
            const quint64 value = parts[i + 1].toULong();

            if (label == QLatin1String("Frequency")) {
                m_coreFrequencyProperty->setValue(value);
            } else if (label == QLatin1String("Render")) {
                m_usageProperty->setValue(value);
            } else if (label == QLatin1String("Video")) {
                if (m_videoUsage) {
                    m_videoUsage->setValue(value);
                }
            } else if (label == QLatin1String("Copy")) {
                if (m_copyUsage) {
                    m_copyUsage->setValue(value);
                }
            } else if (label == QLatin1String("Enhance")) {
                if (m_enhanceUsage) {
                    m_enhanceUsage->setValue(value);
                }
            }
        }
    }
}

#include "moc_LinuxXeGpu.cpp"
