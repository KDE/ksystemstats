/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef CPUPLUGIN_P_H
#define CPUPLUGIN_P_H

class CpuPlugin;
namespace KSysGuard
{
    class SensorContainer;
}

class CpuPluginPrivate {
public:
    CpuPluginPrivate(CpuPlugin *q);
    virtual ~CpuPluginPrivate() =  default;
    virtual void update() {};

    KSysGuard::SensorContainer *m_container;
};

#endif
