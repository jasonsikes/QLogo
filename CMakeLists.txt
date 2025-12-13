cmake_minimum_required(VERSION 3.16)

project(QLogo VERSION 0.961 LANGUAGES CXX C)

include(GNUInstallDirs)

set(QLOGO_BIN qlogo)
set(QLOGO_GUI Psychi)
set(QLOGO_HELP qlogo_help.db)
set(QLOGO_LIB qlogo_library.db)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_BUILD_TYPE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Add debugging flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Svg)

set(QLOGO_DATA_DIR "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/${QLOGO_BIN}")
set(QLOGO_HELP_DB "${QLOGO_DATA_DIR}/${QLOGO_HELP}")
set(QLOGO_LIB_DB "${QLOGO_DATA_DIR}/${QLOGO_LIB}")

add_subdirectory("${QLOGO_BIN}")

add_subdirectory("${QLOGO_GUI}")

file(MAKE_DIRECTORY "${QLOGO_DATA_DIR}")

file(COPY "share/${QLOGO_HELP}" "share/${QLOGO_LIB}"
    DESTINATION "${QLOGO_DATA_DIR}"
)

file(COPY "${CMAKE_SOURCE_DIR}/test"
    DESTINATION "${CMAKE_BINARY_DIR}"
)

file(COPY "${CMAKE_SOURCE_DIR}/blacklisted_tests"
    DESTINATION "${CMAKE_BINARY_DIR}"
)

install(TARGETS "${QLOGO_GUI}" "${QLOGO_BIN}"
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    RESOURCE DESTINATION ${CMAKE_INSTALL_DATADIR}
)

install(DIRECTORY "${QLOGO_DATA_DIR}"
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

if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE ${QLOGO_GUI}.icns)
    # Copy the qlogo binary and help database into the GUI bundle
    set(QLOGO_BIN_LOC ${CMAKE_BINARY_DIR}/${QLOGO_BIN}/${QLOGO_BIN})
    set(QLOGO_BUNDLE_EXE_DIR ${CMAKE_BINARY_DIR}/${QLOGO_GUI}/${QLOGO_GUI}.app/Contents/MacOS)
    set(QLOGO_BUNDLE_RESOURCES_DIR ${CMAKE_BINARY_DIR}/${QLOGO_GUI}/${QLOGO_GUI}.app/Contents/Resources)
    add_custom_target(QLogoBundleCp ALL
        COMMAND cp "${QLOGO_BIN_LOC}" "${QLOGO_BUNDLE_EXE_DIR}"
        COMMAND cp "${QLOGO_HELP_DB}" "${QLOGO_LIB_DB}" "${QLOGO_BUNDLE_RESOURCES_DIR}"
        DEPENDS "${QLOGO_GUI}" "${QLOGO_BIN}"
    )
endif()

