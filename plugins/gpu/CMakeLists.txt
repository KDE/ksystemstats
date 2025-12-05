# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

add_library(ksystemstats_plugin_gpu MODULE GpuPlugin.cpp GpuBackend.cpp GpuDevice.cpp AllGpus.cpp)
target_link_libraries(ksystemstats_plugin_gpu Qt::Core KF6::CoreAddons KF6::I18n KSysGuard::SystemStats KSysGuard::ProcessCore UDev::UDev)

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_link_libraries(ksystemstats_plugin_gpu ${DEVINFO_LIBRARIES})
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    include(CheckIncludeFile)
    check_include_file("drm/xe_drm.h" HAVE_XE_DRM_H)

    target_sources(ksystemstats_plugin_gpu PRIVATE LinuxAmdGpu.cpp LinuxIntelGpu.cpp LinuxNvidiaGpu.cpp LinuxBackend.cpp NvidiaGpu.cpp NvidiaSmiProcess.cpp)

    if(HAVE_XE_DRM_H)
        target_sources(ksystemstats_plugin_gpu PRIVATE LinuxXeGpu.cpp)
        target_compile_definitions(ksystemstats_plugin_gpu PRIVATE HAVE_XE_DRM_H)

        add_executable(ksystemstats_xe_helper XeHelper.cpp)
        configure_file(XeHelperLocation.h.cmake XeHelperLocation.h)
        install(TARGETS ksystemstats_xe_helper DESTINATION ${KDE_INSTALL_LIBEXECDIR})
    endif()

    add_executable(ksystemstats_intel_helper IntelHelper.cpp)
    configure_file(IntelHelperLocation.h.cmake IntelHelperLocation.h)
    install(TARGETS ksystemstats_intel_helper DESTINATION ${KDE_INSTALL_LIBEXECDIR})

    if (Libcap_FOUND)
        install(
            CODE "execute_process(
            COMMAND ${SETCAP_EXECUTABLE}
            CAP_PERFMON=+ep
            \$ENV{DESTDIR}${KDE_INSTALL_FULL_LIBEXECDIR}/ksystemstats_intel_helper)"
        )
        if(HAVE_XE_DRM_H)
            install(
                CODE "execute_process(
                COMMAND ${SETCAP_EXECUTABLE}
                CAP_PERFMON=+ep
                \$ENV{DESTDIR}${KDE_INSTALL_FULL_LIBEXECDIR}/ksystemstats_xe_helper)"
            )
        endif()
    endif()
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD")
    target_sources(ksystemstats_plugin_gpu PRIVATE FreeBSDBackend.cpp FreeBSDNvidiaGpu.cpp NvidiaGpu.cpp NvidiaSmiProcess.cpp)
endif()

if (BUILD_TESTING)
   add_subdirectory(autotests)
endif()

ecm_qt_declare_logging_category(ksystemstats_plugin_gpu HEADER debug.h
    IDENTIFIER KSYSTEMSTATS_GPU
    CATEGORY_NAME org.kde.ksystemstats.gpu
    DESCRIPTION "KSystemStats GPU Plugin"
    EXPORT KSYSTEMSTATS
)

install(TARGETS ksystemstats_plugin_gpu DESTINATION ${KSYSTEMSTATS_PLUGIN_INSTALL_DIR})

