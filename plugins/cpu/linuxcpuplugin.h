/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/
#ifndef LINUXCPUPLUGIN_H
#define LINUXCPUPLUGIN_H

#include <QHash>

#include "cpuplugin_p.h"

struct sensors_chip_name;
class LinuxCpuObject;
class LinuxAllCpusObject;

class LinuxCpuPluginPrivate : public CpuPluginPrivate {
public:
    LinuxCpuPluginPrivate(CpuPlugin *q);
    void update() override;
private:
    void addSensors();
    void addSensorsIntel(const sensors_chip_name * const chipName);
    void addSensorsAmd(const sensors_chip_name * const chipName);

    LinuxAllCpusObject *m_allCpus;
    QVector<LinuxCpuObject *> m_cpus;
    QMultiHash<QPair<unsigned int, unsigned int>, LinuxCpuObject * const> m_cpusBySystemIds;
};

#endif
