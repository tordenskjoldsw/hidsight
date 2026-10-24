# SPDX-FileCopyrightText: 2026 Tobias Kaminski
# SPDX-License-Identifier: MIT

install(PROGRAMS de.tordenskjold.hidsight.desktop DESTINATION ${KDE_INSTALL_APPDIR})
install(FILES de.tordenskjold.hidsight.metainfo.xml DESTINATION ${KDE_INSTALL_METAINFODIR})
install(FILES icons/de.tordenskjold.hidsight.svg DESTINATION ${KDE_INSTALL_ICONDIR}/hicolor/scalable/apps)
install(FILES 70-hidsight.rules DESTINATION ${HIDSIGHT_UDEV_RULES_DIR})
install(FILES hidsight.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})

if(BUILD_TESTING)
    # ECM's appstreamtest only checks installed files; these check the
    # sources, so a broken file fails before anything is installed.
    find_program(APPSTREAMCLI_EXECUTABLE appstreamcli REQUIRED)
    find_program(DESKTOP_FILE_VALIDATE_EXECUTABLE desktop-file-validate REQUIRED)

    add_test(NAME metainfo-validate
        COMMAND ${APPSTREAMCLI_EXECUTABLE} validate --no-net --pedantic
                ${CMAKE_CURRENT_SOURCE_DIR}/de.tordenskjold.hidsight.metainfo.xml)
    add_test(NAME desktop-file-validate
        COMMAND ${DESKTOP_FILE_VALIDATE_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/de.tordenskjold.hidsight.desktop)
endif()
