# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

# Everything but main and QML, so the tests can link it.
add_library(hidsight_app STATIC
    devicemanager.cpp devicemanager.h
    devicemodel.cpp devicemodel.h
    devicemonitor.cpp devicemonitor.h
)

ecm_qt_declare_logging_category(hidsight_app
    HEADER app_logging.h
    IDENTIFIER HIDSIGHT_APP
    CATEGORY_NAME hidsight.app
    DESCRIPTION "Hidsight application"
    EXPORT HIDSIGHT
)

target_include_directories(hidsight_app PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(hidsight_app PUBLIC hidsight_hidpp Qt6::Core PRIVATE UDev::UDev)

add_executable(hidsight main.cpp)

# The QML files live in the top-level qml/ directory; the alias keeps their
# resource paths flat, as if they were next to this file.
set(HIDSIGHT_QML_FILES ${PROJECT_SOURCE_DIR}/qml/Main.qml)
foreach(qml_file IN LISTS HIDSIGHT_QML_FILES)
    get_filename_component(qml_name ${qml_file} NAME)
    set_source_files_properties(${qml_file} PROPERTIES QT_RESOURCE_ALIAS ${qml_name})
endforeach()

qt_add_qml_module(hidsight
    URI de.tordenskjold.hidsight
    VERSION 1.0
    QML_FILES ${HIDSIGHT_QML_FILES}
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
    KF6::I18n
    KF6::I18nQml
)

install(TARGETS hidsight ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
