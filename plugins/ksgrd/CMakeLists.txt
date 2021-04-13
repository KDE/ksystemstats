# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2019 David Edmundson <davidedmundson@kde.org>
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

set(KSYSGUARD_KSGRD_PLUGIN_SOURCES
    ksgrdiface.cpp
)

add_library(ksysguard_ksgrd MODULE ${KSYSGUARD_KSGRD_PLUGIN_SOURCES})
target_link_libraries(ksysguard_ksgrd Qt::Core Qt::DBus KSysGuard::SystemStats KSysGuard::SysGuard KF5::CoreAddons KF5::I18n)

install(TARGETS ksysguard_ksgrd DESTINATION ${KDE_INSTALL_PLUGINDIR}/ksysguard)
