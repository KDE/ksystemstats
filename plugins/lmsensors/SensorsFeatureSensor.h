/*
 * SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SENSORSFEATURESENSOR_H
#define SENSORSFEATURESENSOR_H

#include <systemstats/SensorProperty.h>

struct sensors_chip_name;
struct sensors_feature;
struct sensors_subfeature;

class SensorsFeatureSensor : public  KSysGuard::SensorProperty
{
    Q_OBJECT
public:
    void update() override;
private:
    SensorsFeatureSensor(const QString &id, const sensors_chip_name * const chipName,
                         const sensors_subfeature * const valueFeature,  KSysGuard::SensorObject *parent);
    const sensors_chip_name * m_chipName;
    const sensors_subfeature * m_valueFeature;
    friend SensorsFeatureSensor* makeSensorsFeatureSensor(const sensors_chip_name * const, const sensors_feature * const,  KSysGuard::SensorObject*);
};

SensorsFeatureSensor* makeSensorsFeatureSensor(const sensors_chip_name * const chipName,
                                               const sensors_feature * const feature,
                                               KSysGuard::SensorObject *parent);

#endif
