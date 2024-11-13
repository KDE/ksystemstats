# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

set(KSYSGUARD_OSINFO_PLUGIN_SOURCES
    osinfo.cpp
)

add_library(ksystemstats_plugin_osinfo MODULE ${KSYSGUARD_OSINFO_PLUGIN_SOURCES})
target_link_libraries(ksystemstats_plugin_osinfo Qt::Core Qt::Gui Qt::DBus KF6::CoreAddons KF6::I18n KSysGuard::SystemStats)

ecm_qt_declare_logging_category(ksystemstats_plugin_osinfo HEADER debug.h
    IDENTIFIER KSYSTEMSTATS_OSINFO
    CATEGORY_NAME org.kde.ksystemstats.osinfo
    DESCRIPTION "KSystemStats OSInfo Plugin"
    EXPORT KSYSTEMSTATS
)

install(TARGETS ksystemstats_plugin_osinfo DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})
