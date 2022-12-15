# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

add_library(ksystemstats_plugin_gpu MODULE GpuPlugin.cpp GpuBackend.cpp GpuDevice.cpp AllGpus.cpp)
target_link_libraries(ksystemstats_plugin_gpu Qt::Core KF5::CoreAddons KF5::I18n KSysGuard::SystemStats UDev::UDev)

set(CMAKE_CXX_STANDARD 20)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
   target_sources(ksystemstats_plugin_gpu PRIVATE LinuxAmdGpu.cpp LinuxIntelGpu.cpp LinuxNvidiaGpu.cpp LinuxBackend.cpp NvidiaSmiProcess.cpp)
endif()

install(TARGETS ksystemstats_plugin_gpu DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})

add_executable(ksystemstats_intel_helper IntelHelper.cpp)
configure_file(IntelHelperLocation.h.cmake IntelHelperLocation.h)
install(TARGETS ksystemstats_intel_helper DESTINATION ${KDE_INSTALL_LIBEXECDIR})
if (Libcap_FOUND)
    install(
        CODE "execute_process(
        COMMAND ${SETCAP_EXECUTABLE}
        CAP_PERFMON=+ep
        \$ENV{DESTDIR}${KDE_INSTALL_FULL_LIBEXECDIR}/ksysguard/ksgrd_network_helper)"
    )
endif()

