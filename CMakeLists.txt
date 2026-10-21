cmake_minimum_required(VERSION 3.16)

project(QLogo VERSION 0.98 LANGUAGES CXX C)

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
        # add_compile_definitions(QLOGO_DEBUG) # Temporarily commented out while I run profiler
    endif()
endif()

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Sql Svg)

set(QLOGO_DATA_DIR "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/${QLOGO_BIN}")
set(QLOGO_HELP_DB "${QLOGO_DATA_DIR}/${QLOGO_HELP}")
set(QLOGO_LIB_DB "${QLOGO_DATA_DIR}/${QLOGO_LIB}")
set(QLOGO_CSLS_SRC "${CMAKE_SOURCE_DIR}/csls")
set(QLOGO_CSLS_DIR "${QLOGO_DATA_DIR}/csls")

file(MAKE_DIRECTORY "${QLOGO_DATA_DIR}")

file(GLOB QLOGO_CSLS_FILES CONFIGURE_DEPENDS "${QLOGO_CSLS_SRC}/*")
if(NOT QLOGO_CSLS_FILES)
    message(FATAL_ERROR "No CSLS files found in ${QLOGO_CSLS_SRC}")
endif()

execute_process(
    COMMAND git describe --tags
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_VARIABLE QLOGO_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT QLOGO_VERSION)
    set(QLOGO_VERSION "unknown")
endif()


qt_add_executable(mklogolib util/mklogolib.cpp)
qt_add_executable(mklogohelp util/mklogohelp.cpp)
target_link_libraries(mklogolib PRIVATE Qt6::Core Qt6::Sql)
target_link_libraries(mklogohelp PRIVATE Qt6::Core Qt6::Sql)
set_target_properties(mklogolib mklogohelp PROPERTIES WIN32_EXECUTABLE FALSE)

file(GLOB QLOGO_COMPILER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/qlogo/compiler/compiler*.cpp"
)
if(NOT QLOGO_COMPILER_SOURCES)
    message(FATAL_ERROR "No compiler*.cpp files found")
endif()

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
            "${CMAKE_SOURCE_DIR}/qlogo/compiler"
            "${QLOGO_LIB_DB}"
    DEPENDS mklogolib ${QLOGO_COMPILER_SOURCES}
    COMMENT "Generating ${QLOGO_LIB} from compiler sources"
    VERBATIM
)
add_custom_target(qlogo_library_db ALL DEPENDS "${QLOGO_LIB_DB}")

add_custom_command(
    OUTPUT "${QLOGO_HELP_DB}"
    COMMAND "${CMAKE_COMMAND}" -E env ${_qt_host_env_args}
            "$<TARGET_FILE:mklogohelp>"
            "${CMAKE_SOURCE_DIR}/qlogo/compiler"
            "${QLOGO_HELP_DB}"
    DEPENDS mklogohelp ${QLOGO_COMPILER_SOURCES}
    COMMENT "Generating ${QLOGO_HELP} from compiler sources"
    VERBATIM
)
add_custom_target(qlogo_help_db ALL DEPENDS "${QLOGO_HELP_DB}")

add_custom_command(
    OUTPUT "${CMAKE_BINARY_DIR}/csls.stamp"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${QLOGO_CSLS_SRC}"
            "${QLOGO_CSLS_DIR}"
    COMMAND "${CMAKE_COMMAND}" -E touch "${CMAKE_BINARY_DIR}/csls.stamp"
    DEPENDS ${QLOGO_CSLS_FILES}
    COMMENT "Copying CSLS files to ${QLOGO_CSLS_DIR}"
    VERBATIM
)
add_custom_target(qlogo_csls ALL DEPENDS "${CMAKE_BINARY_DIR}/csls.stamp")

add_subdirectory("${QLOGO_BIN}")

add_subdirectory("${QLOGO_GUI}")

add_dependencies("${QLOGO_BIN}" qlogo_library_db qlogo_help_db qlogo_csls)
add_dependencies("${QLOGO_GUI}" qlogo_library_db qlogo_help_db qlogo_csls)

option(QLOGO_ENABLE_FUZZING "Build libFuzzer targets" OFF)
if(QLOGO_ENABLE_FUZZING)
    add_subdirectory(tests/fuzz)
endif()

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
    install(DIRECTORY "${QLOGO_CSLS_SRC}/"
        DESTINATION "${CMAKE_INSTALL_BINDIR}/csls"
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

