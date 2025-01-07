# SPDX-License-Identifier: BSD-2-Clause
# SPDX-FileCopyrightText: 2024 Henry Hu

# - Try to find the devinfo directory library
# Once done this will define
#
#  DEVINFO_FOUND - system has DEVINFO
#  DEVINFO_INCLUDE_DIR - the DEVINFO include directory
#  DEVINFO_LIBRARIES - The libraries needed to use DEVINFO

FIND_PATH(DEVINFO_INCLUDE_DIR devinfo.h)

FIND_LIBRARY(DEVINFO_LIBRARIES NAMES devinfo)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(devinfo DEFAULT_MSG DEVINFO_INCLUDE_DIR DEVINFO_LIBRARIES )

MARK_AS_ADVANCED(DEVINFO_INCLUDE_DIR DEVINFO_LIBRARIES)

