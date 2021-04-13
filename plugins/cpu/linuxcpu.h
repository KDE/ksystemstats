/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.1-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef LINUXCPU_H
#define LINUXCPU_H

struct sensors_chip_name;
struct sensors_feature;

#include "cpu.h"
#include "usagecomputer.h"


class TemperatureSensor : public KSysGuard::SensorProperty {
public:
    TemperatureSensor(const QString &id, KSysGuard::SensorObject *parent);
    void setFeature(const sensors_chip_name * const chipName, const sensors_feature * const feature);
    void update() override;
private:
    const sensors_chip_name * m_sensorChipName;
    int m_temperatureSubfeature;
};

class LinuxCpuObject : public CpuObject
{
public:
    LinuxCpuObject(const QString &id, const QString &name, KSysGuard::SensorContainer *parent);

    void update(unsigned long long system, unsigned long long user, unsigned long long wait, unsigned long long idle);
    TemperatureSensor* temperatureSensor();
    void initialize(double initialFrequency);
private:
    void initialize() override {};
    void makeSensors() override;
    UsageComputer m_usageComputer;
    TemperatureSensor *m_temperatureSensor;
};

class LinuxAllCpusObject : public AllCpusObject {
public:
    using AllCpusObject::AllCpusObject;
    void update(unsigned long long system, unsigned long long user, unsigned long long wait, unsigned long long idle);
private:
    UsageComputer m_usageComputer;
};

#endif
