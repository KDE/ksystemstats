# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

set(KSYSGUARD_OSINFO_PLUGIN_SOURCES
    osinfo.cpp
)

add_library(ksysguard_plugin_osinfo MODULE ${KSYSGUARD_OSINFO_PLUGIN_SOURCES})
target_link_libraries(ksysguard_plugin_osinfo Qt::Core Qt::Gui Qt::DBus KF5::CoreAddons KF5::I18n KSysGuard::SystemStats)

install(TARGETS ksysguard_plugin_osinfo DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})
