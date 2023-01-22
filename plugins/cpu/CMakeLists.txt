# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2020 David Redondo <kde@david-redondo.de>
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

add_library(ksystemstats_plugin_cpu MODULE  cpu.cpp cpuplugin.cpp cpuplugin.h loadaverages.cpp loadaverages.h usagecomputer.cpp usagecomputer.h)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(ksystemstats_plugin_cpu PRIVATE linuxcpu.cpp linuxcpu.h linuxcpuplugin.cpp linuxcpuplugin.h)
elseif(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_sources(ksystemstats_plugin_cpu PRIVATE freebsdcpuplugin.cpp freebsdcpuplugin.h)
endif()

target_link_libraries(ksystemstats_plugin_cpu Qt::Core KF5::CoreAddons KF5::I18n KSysGuard::SystemStats)

install(TARGETS ksystemstats_plugin_cpu DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})
