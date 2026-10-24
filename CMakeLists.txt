# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

cmake_minimum_required(VERSION 3.25)

project(hidsight VERSION 0.0.0 LANGUAGES CXX)

# The minimum versions are the ones on the maintainer's desk: nothing older
# has been built or run (PLAN.md section 12).
set(QT_MIN_VERSION "6.12.0")
set(KF_MIN_VERSION "6.31.0")

find_package(ECM ${KF_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDEClangFormat)
include(ECMSetupVersion)
include(ECMEnableSanitizers)
include(FeatureSummary)

ecm_setup_version(${PROJECT_VERSION}
    VARIABLE_PREFIX HIDSIGHT
    VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/hidsight-version.h
)

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2 Widgets)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS CoreAddons I18n)

find_package(KF6Kirigami ${KF_MIN_VERSION})
set_package_properties(KF6Kirigami PROPERTIES
    TYPE RUNTIME
    DESCRIPTION "QML components of the window"
)
find_package(KF6QQC2DesktopStyle ${KF_MIN_VERSION})
set_package_properties(KF6QQC2DesktopStyle PROPERTIES
    TYPE RUNTIME
    DESCRIPTION "Qt Quick Controls style that follows the Plasma theme"
)

qt_policy(SET QTP0001 NEW)
qt_policy(SET QTP0004 NEW)

set(HIDSIGHT_UDEV_RULES_DIR "lib/udev/rules.d" CACHE PATH
    "Directory for the udev rule, relative to the install prefix")

add_subdirectory(src/app)
add_subdirectory(data)

file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.h
    ${PROJECT_SOURCE_DIR}/tests/*.cpp ${PROJECT_SOURCE_DIR}/tests/*.h)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

feature_summary(WHAT ALL INCLUDE_QUIET_PACKAGES FATAL_ON_MISSING_REQUIRED_PACKAGES)
