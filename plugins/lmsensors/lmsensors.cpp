/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "lmsensors.h"

#include "SensorsFeatureSensor.h"

#include <systemstats/SensorContainer.h>
#include <systemstats/SensorObject.h>

#include <KLocalizedString>
#include <KPluginFactory>

#include <sensors/sensors.h>

LmSensorsPlugin::LmSensorsPlugin(QObject *parent, const QVariantList &args)
    : KSysGuard::SensorPlugin(parent, args)
{
    auto container = new KSysGuard::SensorContainer(QStringLiteral("lmsensors"), i18n( "Hardware Sensors" ), this);

    const std::array<QByteArray, 3> blacklist{"coretemp","k10temp","amdgpu"}; //already handled by other plugins
    int chipNumber = 0;
    while (const sensors_chip_name * const chipName = sensors_get_detected_chips(nullptr, &chipNumber)) {
        if (std::find(blacklist.cbegin(), blacklist.cend(), chipName->prefix) != blacklist.cend()) {
            continue;
        }
        int requiredBytes  = sensors_snprintf_chip_name(nullptr, 0, chipName) + 1;
        QByteArray name;
        name.resize(requiredBytes);
        sensors_snprintf_chip_name(name.data(), name.size(), chipName);
        const QString nameString = QString::fromUtf8(name);
        KSysGuard::SensorObject *sensorObject = container->object(nameString);
        if (!sensorObject) {
            sensorObject = new KSysGuard::SensorObject(nameString, nameString, container);
        }
        int featureNumber = 0;
        while (const sensors_feature * const feature = sensors_get_features(chipName, &featureNumber)) {
            if (auto sensor = makeSensorsFeatureSensor(chipName, feature, sensorObject)) {
                m_sensors.push_back(sensor);
            }
        }
    }
}

LmSensorsPlugin::~LmSensorsPlugin()
{
}


QString LmSensorsPlugin::providerName() const
{
    return QStringLiteral("lmsensors");
}

void LmSensorsPlugin::update()
{
    for (auto sensor : qAsConst(m_sensors)) {
        sensor->update();
    }
}

K_PLUGIN_CLASS_WITH_JSON(LmSensorsPlugin, "metadata.json")
#include "lmsensors.moc"