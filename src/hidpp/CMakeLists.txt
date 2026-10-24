# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

add_library(hidsight_hidpp STATIC
    channel.h
    dispatcher.cpp dispatcher.h
    hidrawchannel.cpp hidrawchannel.h
    report.cpp report.h
    result.h
)

ecm_qt_declare_logging_category(hidsight_hidpp
    HEADER hidpp_logging.h
    IDENTIFIER HIDSIGHT_HIDPP
    CATEGORY_NAME hidsight.hidpp
    DESCRIPTION "Hidsight HID++ protocol"
    EXPORT HIDSIGHT
)
ecm_qt_declare_logging_category(hidsight_hidpp
    HEADER raw_logging.h
    IDENTIFIER HIDSIGHT_RAW
    CATEGORY_NAME hidsight.raw
    DESCRIPTION "Hidsight raw HID++ reports"
    EXPORT HIDSIGHT
)

target_include_directories(hidsight_hidpp PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(hidsight_hidpp PUBLIC Qt6::Core)
