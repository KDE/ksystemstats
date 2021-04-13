/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3 of
    the license or (at your option) at any later version that is
    accepted by the membership of KDE e.V. (or its successor
    approved by the membership of KDE e.V.), which shall act as a
    proxy as defined in Section 14 of version 3 of the license.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.
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
