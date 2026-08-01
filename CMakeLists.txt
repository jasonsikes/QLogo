cmake_minimum_required(VERSION 3.16)

project(QLogo VERSION 0.961 LANGUAGES CXX C)

include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(QLOGO_BIN qlogo)
set(QLOGO_GUI Psychi)
set(QLOGO_HELP qlogo_help.db)
set(QLOGO_LIB qlogo_library.db)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (CMAKE_BUILD_TYPE)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        if(APPLE)
            set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
        endif()
    elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_definitions(QLOGO_DEBUG)
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

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

file(COPY "${CMAKE_SOURCE_DIR}/tests"
    DESTINATION "${CMAKE_BINARY_DIR}"
)

install(TARGETS "${QLOGO_GUI}" "${QLOGO_BIN}"
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    RESOURCE DESTINATION ${CMAKE_INSTALL_DATADIR}
)

if(WIN32)
    install(FILES "${QLOGO_HELP_DB}" "${QLOGO_LIB_DB}"
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    )
elseif(UNIX AND NOT APPLE)
    # On Linux, findDBPath uses ../share/qlogo relative to the binary.
    install(DIRECTORY "${QLOGO_DATA_DIR}"
        DESTINATION "${CMAKE_INSTALL_DATADIR}"
    )
endif()
# On macOS the DBs live in the Psychi.app bundle
if(UNIX AND NOT APPLE)
    install(FILES "${QLOGO_GUI}.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications"
    )

    install(DIRECTORY "icons/linux/hicolor"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/icons"
    )

    install(FILES "${QLOGO_GUI}.appdata.xml"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo"
    )
endif()

if (APPLE)
    set(MACOSX_BUNDLE_ICON_FILE ${QLOGO_GUI}.icns)
    # Copy the qlogo binary into the GUI bundle.
    set(QLOGO_BIN_LOC ${CMAKE_BINARY_DIR}/${QLOGO_BIN}/${QLOGO_BIN})
    set(QLOGO_BUNDLE_EXE_DIR ${CMAKE_BINARY_DIR}/${QLOGO_GUI}/${QLOGO_GUI}.app/Contents/MacOS)
    add_custom_target(QLogoBundleCp ALL
        COMMAND "${CMAKE_COMMAND}" -E copy "${QLOGO_BIN_LOC}" "${QLOGO_BUNDLE_EXE_DIR}"
        DEPENDS "${QLOGO_GUI}" "${QLOGO_BIN}"
    )
endif()

