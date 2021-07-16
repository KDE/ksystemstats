/*
    SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/
#ifndef LINUXCPUPLUGIN_H
#define LINUXCPUPLUGIN_H

#include <QHash>

#include "cpuplugin_p.h"

struct sensors_chip_name;
class LinuxCpuObject;
class LinuxAllCpusObject;
class LoadAverages;

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
    LoadAverages *m_loadAverages;
};

#endif
