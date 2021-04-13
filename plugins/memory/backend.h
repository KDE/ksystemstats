/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.1-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#ifndef BACKEND_H
#define BACKEND_H

namespace KSysGuard {
    class SensorContainer;
    class SensorObject;
    class SensorProperty;
}

class MemoryBackend {
public:
    MemoryBackend(KSysGuard::SensorContainer *container);
    virtual ~MemoryBackend() = default;

    void initSensors();
    virtual void update() = 0;
protected:
    virtual void makeSensors();

    KSysGuard::SensorProperty *m_total;
    KSysGuard::SensorProperty *m_used;
    KSysGuard::SensorProperty *m_free;
    KSysGuard::SensorProperty *m_application;
    KSysGuard::SensorProperty *m_cache;
    KSysGuard::SensorProperty *m_buffer;
    KSysGuard::SensorProperty *m_swapTotal;
    KSysGuard::SensorProperty *m_swapUsed;
    KSysGuard::SensorProperty *m_swapFree;
    KSysGuard::SensorObject *m_physicalObject;
    KSysGuard::SensorObject *m_swapObject;
};

#endif
