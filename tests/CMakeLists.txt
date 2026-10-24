# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

ecm_add_tests(
    capturetest.cpp
    descriptortest.cpp
    dispatchertest.cpp
    parsertest.cpp
    reporttest.cpp
    LINK_LIBRARIES hidsight_hidpp Qt6::Test
)

target_compile_definitions(capturetest PRIVATE CAPTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/captures")
