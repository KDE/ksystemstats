# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

add_library(ksystemstats_plugin_gpu MODULE GpuPlugin.cpp GpuPlugin.h GpuBackend.cpp GpuBackend.h GpuDevice.cpp GpuDevice.h AllGpus.cpp AllGpus.h)
target_link_libraries(ksystemstats_plugin_gpu Qt::Core KF5::CoreAddons KF5::I18n KSysGuard::SystemStats UDev::UDev)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   target_sources(ksystemstats_plugin_gpu PRIVATE LinuxAmdGpu.cpp LinuxAmdGpu.h LinuxNvidiaGpu.cpp LinuxNvidiaGpu.h LinuxBackend.cpp LinuxBackend.h NvidiaSmiProcess.cpp NvidiaSmiProcess.h)
endif()

install(TARGETS ksystemstats_plugin_gpu DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})
