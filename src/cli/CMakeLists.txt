# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

add_executable(hidsight-cli main.cpp listing.cpp)

target_include_directories(hidsight-cli PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(hidsight-cli PRIVATE
    hidsight_hidpp
    KF6::CoreAddons
    KF6::I18n
)

install(TARGETS hidsight-cli ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
