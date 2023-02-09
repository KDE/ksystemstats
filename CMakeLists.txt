# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

cmake_minimum_required(VERSION 3.16)

project(ksystemstats)
set(PROJECT_VERSION "5.27.0")

set(QT_MIN_VERSION "5.15.2")
set(KF5_MIN_VERSION "5.102.0")
set(KDE_COMPILERSETTINGS_LEVEL "5.82")

find_package(ECM ${KF5_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH}  ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CheckIncludeFiles)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(ECMAddTests)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMConfiguredInstall)
include(ECMGenerateDBusServiceFile)
include(ECMDeprecationSettings)

find_package(Qt${QT_MAJOR_VERSION} ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Core Test)
find_package(KF5 ${KF5_MIN_VERSION} REQUIRED COMPONENTS CoreAddons DBusAddons Solid KIO)
find_package(KSysGuard REQUIRED)

find_package(KF5NetworkManagerQt ${KF5_MIN_VERSION})
set_package_properties(KF5NetworkManagerQt PROPERTIES
    TYPE RECOMMENDED
    PURPOSE "Provides an improved backend for Network statistics"
)

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)
add_definitions(-DQT_USE_QSTRINGBUILDER)

ecm_set_disabled_deprecation_versions(QT 5.15.2
    KF 5.101
)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(Sensors)
    set_package_properties(Sensors PROPERTIES
            TYPE REQUIRED
            PURPOSE "Used for reading hardware sensors that aren't covered by other plugins."
            URL "https://github.com/lm-sensors/lm-sensors"
        )

    find_package(NL)
    set_package_properties(NL PROPERTIES
        TYPE REQUIRED
        PURPOSE "Used for gathering socket info via the sock_diag netlink subsystem and for the network plugin when NetworkManagerQt is not available."
        URL "https://github.com/thom311/libnl/"
    )
endif()

find_package(UDev)
set_package_properties(
    UDev PROPERTIES
    TYPE RECOMMENDED
    PURPOSE "UDev is used for finding graphics cards."
)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

add_subdirectory(src)
add_subdirectory(plugins)
add_subdirectory(tools/kstatsviewer)

if (BUILD_TESTING)
    add_subdirectory(autotests)
endif()

# add clang-format target for all our real source files
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

ki18n_install(po)

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
