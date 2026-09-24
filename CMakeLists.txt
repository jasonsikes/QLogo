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

file(MAKE_DIRECTORY "${QLOGO_DATA_DIR}")

qt_add_executable(mklogolib util/mklogolib.cpp)
qt_add_executable(mklogohelp util/mklogohelp.cpp)
target_link_libraries(mklogolib PRIVATE Qt6::Core Qt6::Sql)
target_link_libraries(mklogohelp PRIVATE Qt6::Core Qt6::Sql)
set_target_properties(mklogolib mklogohelp PROPERTIES WIN32_EXECUTABLE FALSE)

file(GLOB LOGOLIB_SOURCES CONFIGURE_DEPENDS LIST_DIRECTORIES false
    "${CMAKE_SOURCE_DIR}/logolib/*"
)
if(NOT LOGOLIB_SOURCES)
    message(FATAL_ERROR "No files found in logolib/")
endif()

file(GLOB QLOGO_COMPILER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/qlogo/compiler/*.cpp"
)
if(NOT QLOGO_COMPILER_SOURCES)
    message(FATAL_ERROR "No compiler .cpp files found")
endif()
set(QLOGO_HELP_JSON "${CMAKE_SOURCE_DIR}/util/help.json")

set(_qt_host_env_args)
if(WIN32)
    get_target_property(_qt_host_qmake Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(_qt_host_bin "${_qt_host_qmake}" DIRECTORY)
    list(APPEND _qt_host_env_args "PATH=${_qt_host_bin};$ENV{PATH}")
endif()

add_custom_command(
    OUTPUT "${QLOGO_LIB_DB}"
    COMMAND "${CMAKE_COMMAND}" -E env ${_qt_host_env_args}
            "$<TARGET_FILE:mklogolib>"
            "${CMAKE_SOURCE_DIR}/logolib"
            "${QLOGO_LIB_DB}"
    DEPENDS mklogolib ${LOGOLIB_SOURCES}
    COMMENT "Generating ${QLOGO_LIB} from logolib"
    VERBATIM
)
add_custom_target(qlogo_library_db ALL DEPENDS "${QLOGO_LIB_DB}")

add_custom_command(
    OUTPUT "${QLOGO_HELP_DB}"
    COMMAND "${CMAKE_COMMAND}" -E env ${_qt_host_env_args}
            "$<TARGET_FILE:mklogohelp>"
            "${CMAKE_SOURCE_DIR}/qlogo/compiler"
            "${QLOGO_HELP_JSON}"
            "${QLOGO_HELP_DB}"
    DEPENDS mklogohelp ${QLOGO_COMPILER_SOURCES} "${QLOGO_HELP_JSON}"
    COMMENT "Generating ${QLOGO_HELP} from compiler sources"
    VERBATIM
)
add_custom_target(qlogo_help_db ALL DEPENDS "${QLOGO_HELP_DB}")

add_subdirectory("${QLOGO_BIN}")

add_subdirectory("${QLOGO_GUI}")

add_dependencies("${QLOGO_BIN}" qlogo_library_db qlogo_help_db)
add_dependencies("${QLOGO_GUI}" qlogo_library_db qlogo_help_db)

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

