# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

add_library(hidsight_hidpp STATIC
    battery.cpp battery.h
    channel.h
    device.cpp device.h
    devicefwversion.cpp devicefwversion.h
    devicename.cpp devicename.h
    dispatcher.cpp dispatcher.h
    hidrawchannel.cpp hidrawchannel.h
    pointer.cpp pointer.h
    probe.cpp probe.h
    receiver.cpp receiver.h
    report.cpp report.h
    result.h
    sequence_p.h
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
