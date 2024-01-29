cmake_minimum_required(VERSION 3.5)

project(QLogo VERSION 0.931 LANGUAGES CXX)

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

file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/share")

set(QLOGO_HELP_DB "${PROJECT_BINARY_DIR}/share/qlogo_help.db")

add_custom_target(QLOGO_BUILD_HELP ALL
    COMMAND "${GEN_HELP_BIN}" "${QLOGO_HELP_DB}" "${CMAKE_SOURCE_DIR}/qlogo/executor"
    DEPENDS "${QLOGO_GUI}" "${QLOGO_BIN}" "${GEN_HELP_BIN}"
)
include(GNUInstallDirs)

install(TARGETS "${QLOGO_GUI}" "${QLOGO_BIN}" "${GEN_HELP_BIN}"
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# On MacOS copy the qlogo binary and help database into the GUI bundle
if (APPLE)
    set(QLOGO_BIN_LOC ${CMAKE_BINARY_DIR}/${QLOGO_BIN}/${QLOGO_BIN})
    set(QLOGO_BUNDLE_EXE_DIR ${CMAKE_BINARY_DIR}/${QLOGO_GUI}/${QLOGO_GUI}.app/Contents/MacOS)
    add_custom_target(QLogoBundleCp ALL
                      COMMAND cp ${QLOGO_BIN_LOC} "${QLOGO_HELP_DB}" ${QLOGO_BUNDLE_EXE_DIR}
                      DEPENDS "${QLOGO_GUI}" "${QLOGO_BIN}" QLOGO_BUILD_HELP
                  )
endif()

# TODO: rm -rf ~/temp/qlogo
# && cmake --install . --prefix ~/temp/qlogo
# && /opt/Qt/6.5.1/macos/bin/macdeployqt /Users/jsikes/temp/qlogo/./QLogo-GUI.app -always-overwrite
