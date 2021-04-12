# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>

add_library(ksysguard_plugin_lmsensors MODULE lmsensors.cpp SensorsFeatureSensor.cpp)

target_include_directories(ksysguard_plugin_lmsensors PRIVATE ${SENSORS_INCLUDE_DIR})
target_link_libraries(ksysguard_plugin_lmsensors PRIVATE Qt5::Core KF5::CoreAddons KF5::I18n  KSysGuard::SystemStats ${SENSORS_LIBRARIES})

install(TARGETS ksysguard_plugin_lmsensors DESTINATION ${KDE_INSTALL_PLUGINDIR}/ksysguard)
