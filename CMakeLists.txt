cmake_minimum_required(VERSION 3.5)

project(QLogo VERSION 0.931 LANGUAGES CXX)

include(GNUInstallDirs)

set(QLOGO_BIN qlogo)
set(QLOGO_GUI QLogo-GUI)
set(GEN_HELP_BIN gen_help)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_BUILD_TYPE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets Sql)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets Sql)

add_subdirectory("${QLOGO_BIN}")

add_subdirectory("${QLOGO_GUI}")

add_subdirectory("util")

set(QLOGO_DB_DIR "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/${QLOGO_BIN}")
set(QLOGO_HELP_DB "${QLOGO_DB_DIR}/qlogo_help.db")
set(QLOGO_HELP_SRC "${CMAKE_SOURCE_DIR}/${QLOGO_BIN}/executor")

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/${QLOGO_BIN}")

add_custom_target(QLOGO_BUILD_HELP
    COMMENT "BUILDING HELP, sources: ${QLOGO_HELP_SRC}  dest: ${QLOGO_HELP_DB}"
    COMMAND "${GEN_HELP_BIN}" "${QLOGO_HELP_DB}" "${QLOGO_HELP_SRC}"
    DEPENDS "${GEN_HELP_BIN}"
)

install(TARGETS "${QLOGO_GUI}" "${QLOGO_BIN}"
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY "${QLOGO_DB_DIR}"
    DESTINATION "${CMAKE_INSTALL_DATADIR}"
)

install(FILES "${QLOGO_GUI}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
)

install(DIRECTORY "icons/hicolor"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
)

install(FILES "${QLOGO_GUI}.appdata.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
)

add_dependencies("${QLOGO_BIN}" QLOGO_BUILD_HELP)

file(COPY "${CMAKE_SOURCE_DIR}/test"
    DESTINATION "${CMAKE_BINARY_DIR}")

if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE QLogo-GUI.icns)
    # Copy the qlogo binary and help database into the GUI bundle
    set(QLOGO_BIN_LOC ${CMAKE_BINARY_DIR}/${QLOGO_BIN}/${QLOGO_BIN})
    set(QLOGO_BUNDLE_EXE_DIR ${CMAKE_BINARY_DIR}/${QLOGO_GUI}/${QLOGO_GUI}.app/Contents/MacOS)
    set(QLOGO_BUNDLE_RESOURCES_DIR ${CMAKE_BINARY_DIR}/${QLOGO_GUI}/${QLOGO_GUI}.app/Contents/Resources)
    add_custom_target(QLogoBundleCp ALL
        COMMAND cp "${QLOGO_BIN_LOC}" "${QLOGO_BUNDLE_EXE_DIR}"
        COMMAND cp "${QLOGO_HELP_DB}" "${QLOGO_BUNDLE_RESOURCES_DIR}"
        DEPENDS "${QLOGO_GUI}" "${QLOGO_BIN}" QLOGO_BUILD_HELP
    )
endif()

# TODO: rm -rf ~/temp/qlogo
# && cmake --install . --prefix ~/temp/qlogo
# && /opt/Qt/6.5.1/macos/bin/macdeployqt /Users/jsikes/temp/qlogo/./QLogo-GUI.app -always-overwrite
