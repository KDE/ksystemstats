/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CPU_H
#define CPU_H

#include <systemstats/SensorObject.h>

class BaseCpuObject : public KSysGuard::SensorObject {
public:
protected:
    BaseCpuObject(const QString &id, const QString &name, KSysGuard::SensorContainer *parent);

    virtual void initialize();
    virtual void makeSensors();

    KSysGuard::SensorProperty *m_usage;
    KSysGuard::SensorProperty *m_system;
    KSysGuard::SensorProperty *m_user;
    KSysGuard::SensorProperty *m_wait;
};

class CpuObject : public BaseCpuObject {
public:
    CpuObject(const QString &id, const QString &name, KSysGuard::SensorContainer *parent);
    void initialize() override;
protected:
    void makeSensors() override;

    KSysGuard::SensorProperty *m_frequency;
    KSysGuard::SensorProperty *m_temperature;
};

class AllCpusObject : public BaseCpuObject {
public:
    AllCpusObject(KSysGuard::SensorContainer *parent);
    void setCounts(unsigned int cpuCount, unsigned int coreCount);
    void initialize() override;
protected:
    void makeSensors() override;

    KSysGuard::SensorProperty *m_cpuCount;
    KSysGuard::SensorProperty *m_coreCount;
};

#endif
