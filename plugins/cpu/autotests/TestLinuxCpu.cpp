/*
    SPDX-FileCopyrightText: 2024 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include <QSignalSpy>
#include <QTest>

#define protected public
#define private public

#include "../cpuplugin.h"
#include "../linuxcpu.h"
#include "../linuxcpuplugin.h"

class LinuxCpuTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testParseCpuinfo_data();
    void testParseCpuinfo();

private:
};

void LinuxCpuTest::testParseCpuinfo_data()
{
    QTest::addColumn<QString>("cpuinfoPath");
    QTest::addColumn<int>("expectedCpuCount");
    QTest::addColumn<int>("expectedCoreCount");
    QTest::addColumn<int>("expectedSensorCount");
    // QTestL:

    QTest::newRow("AMD64")
        << QFINDTESTDATA("fixtures/linux_amd64_cpuinfo.txt")
        << 1
        << 12
        << 12
        ;
    QTest::newRow("ARM")
        << QFINDTESTDATA("fixtures/linux_arm_cpuinfo.txt")
        << 1
        << 8
        << 8
        ;
}

void LinuxCpuTest::testParseCpuinfo()
{
    QFETCH(QString, cpuinfoPath);

    CpuPlugin plugin{nullptr, QVariantList{}};
    LinuxCpuPluginPrivate cpu(&plugin, cpuinfoPath);

    QFETCH(int, expectedCpuCount);
    QCOMPARE(cpu.m_allCpus->m_cpuCount->value(), expectedCpuCount);

    QFETCH(int, expectedCoreCount);
    QCOMPARE(cpu.m_allCpus->m_coreCount->value(), expectedCoreCount);

    QFETCH(int, expectedSensorCount);
    QCOMPARE(cpu.m_cpus.size(), expectedSensorCount);
}

QTEST_MAIN(LinuxCpuTest)

#include "TestLinuxCpu.moc"
