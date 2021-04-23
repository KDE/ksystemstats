# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 David Redondo <kde@david-redondo.de>

add_library(ksystemstats_plugin_lmsensors MODULE lmsensors.cpp SensorsFeatureSensor.cpp)

target_include_directories(ksystemstats_plugin_lmsensors PRIVATE ${SENSORS_INCLUDE_DIR})
target_link_libraries(ksystemstats_plugin_lmsensors PRIVATE Qt5::Core KF5::CoreAddons KF5::I18n  KSysGuard::SystemStats ${SENSORS_LIBRARIES})

install(TARGETS ksystemstats_plugin_lmsensors DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})

