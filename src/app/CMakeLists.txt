# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

# Everything but main and QML, so the tests can link it.
add_library(hidsight_app STATIC
    appsettings.cpp appsettings.h
    autostart.cpp autostart.h
    batteryalerts.cpp batteryalerts.h
    batterytray.cpp batterytray.h
    devicedetails.cpp devicedetails.h
    devicemanager.cpp devicemanager.h
    devicemodel.cpp devicemodel.h
    devicemonitor.cpp devicemonitor.h
    devicenames.cpp devicenames.h
    devicetexts.cpp devicetexts.h
    notifier.cpp notifier.h
    traylook.cpp traylook.h
)

ecm_qt_declare_logging_category(hidsight_app
    HEADER app_logging.h
    IDENTIFIER HIDSIGHT_APP
    CATEGORY_NAME hidsight.app
    DESCRIPTION "Hidsight application"
    EXPORT HIDSIGHT
)

target_include_directories(hidsight_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hidsight_app
    PUBLIC hidsight_hidpp Qt6::Core KF6::ConfigCore
    PRIVATE UDev::UDev Qt6::DBus Qt6::Widgets KF6::CoreAddons KF6::I18n KF6::Notifications KF6::StatusNotifierItem
)
if(HIDSIGHT_DEVELOPER)
    target_compile_definitions(hidsight_app PUBLIC HIDSIGHT_DEVELOPER)
endif()

add_executable(hidsight main.cpp qmltypes.cpp qmltypes.h)

# The QML files live in the top-level qml/ directory; the alias keeps their
# resource paths flat, as if they were next to this file.
set(HIDSIGHT_QML_FILES
    ${PROJECT_SOURCE_DIR}/qml/Main.qml
    ${PROJECT_SOURCE_DIR}/qml/DevicesPage.qml
    ${PROJECT_SOURCE_DIR}/qml/DevicePage.qml
    ${PROJECT_SOURCE_DIR}/qml/SettingsPage.qml
    ${PROJECT_SOURCE_DIR}/qml/BatteryRing.qml
    ${PROJECT_SOURCE_DIR}/qml/DeviceCard.qml
    ${PROJECT_SOURCE_DIR}/qml/StatusChips.qml
    ${PROJECT_SOURCE_DIR}/qml/DeviceIllustration.qml
    ${PROJECT_SOURCE_DIR}/qml/Tones.qml
    ${PROJECT_SOURCE_DIR}/qml/Pill.qml
)
set_source_files_properties(${PROJECT_SOURCE_DIR}/qml/Tones.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

set(HIDSIGHT_ILLUSTRATIONS
    illustrations/keyboard-dark.svg
    illustrations/keyboard-light.svg
    illustrations/mouse-dark.svg
    illustrations/mouse-light.svg
)
set(HIDSIGHT_RESOURCE_FILES)
foreach(illustration IN LISTS HIDSIGHT_ILLUSTRATIONS)
    set_source_files_properties(${PROJECT_SOURCE_DIR}/qml/${illustration} PROPERTIES QT_RESOURCE_ALIAS ${illustration})
    list(APPEND HIDSIGHT_RESOURCE_FILES ${PROJECT_SOURCE_DIR}/qml/${illustration})
endforeach()
foreach(qml_file IN LISTS HIDSIGHT_QML_FILES)
    get_filename_component(qml_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${qml_name})
endforeach()

qt_add_qml_module(hidsight
    URI de.tordenskjold.hidsight
    VERSION 1.0
    QML_FILES ${HIDSIGHT_QML_FILES}
    RESOURCES ${HIDSIGHT_RESOURCE_FILES}
)

target_include_directories(hidsight PRIVATE ${PROJECT_BINARY_DIR})

target_link_libraries(hidsight PRIVATE
    hidsight_app
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    KF6::CoreAddons
    KF6::DBusAddons
    KF6::I18n
    KF6::I18nQml
)

install(TARGETS hidsight ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
