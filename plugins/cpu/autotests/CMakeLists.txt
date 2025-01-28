# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Arjen Hiemstra <ahiemstra@heimr.nl>

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    get_target_property(_bindir ksystemstats_plugin_cpu BINARY_DIR)

    ecm_add_test(
        TestLinuxCpu.cpp
        ../cpu.cpp
        ../cpuplugin.cpp
        ../linuxcpu.cpp
        ../linuxcpuplugin.cpp
        ../loadaverages.cpp
        ../usagecomputer.cpp
        ${_bindir}/debug.cpp
        TEST_NAME TestLinuxCpu
        LINK_LIBRARIES Qt::Test KF6::CoreAddons KSysGuard::SystemStats
    )
    target_include_directories(TestLinuxCpu PRIVATE ${_bindir})
endif()
