# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

ecm_add_tests(
    capturetest.cpp
    descriptortest.cpp
    dispatchertest.cpp
    parsertest.cpp
    reporttest.cpp
    sessiontest.cpp
    LINK_LIBRARIES hidsight_hidpp Qt6::Test
)

ecm_add_tests(
    alertstest.cpp
    devicemodeltest.cpp
    LINK_LIBRARIES hidsight_app KF6::I18n Qt6::Test
)

foreach(test alertstest capturetest sessiontest)
    target_compile_definitions(${test} PRIVATE CAPTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/captures")
endforeach()
