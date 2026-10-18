# Technically, the qlogo resource files belong to the qlogo binary,
# but on macOS they live in the GUI bundle's Contents/Resources so
# findDBPath (../Resources/) can find them when qlogo runs from Psychi.
set(RESOURCE_FILES
  "${QLOGO_HELP_DB}"
  "${QLOGO_LIB_DB}"
)
set_source_files_properties("${QLOGO_HELP_DB}" "${QLOGO_LIB_DB}" PROPERTIES GENERATED TRUE)
set_source_files_properties(${QLOGO_CSLS_FILES} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources/csls")
set(GUI_SOURCES
  canvas.cpp
  console.cpp
  editorwindow.cpp
  mainwindow.cpp
  psychi_main.cpp

  ../include/gui/canvas.h
  ../include/gui/console.h
  ../include/gui/editorwindow.h
  ../include/gui/mainwindow.h
  ../include/sharedconstants.h

  editorwindow.ui
  mainwindow.ui
)

find_package(Qt6 REQUIRED COMPONENTS Sql)

if(WIN32)
    set(app_icon_resource_windows "${CMAKE_SOURCE_DIR}/icons/windows/${QLOGO_GUI}.rc")
    qt_add_executable("${QLOGO_GUI}"
      ${GUI_SOURCES}
      ${app_icon_resource_windows}
    )
elseif(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE "${QLOGO_GUI}.icns")
    set(app_icon_macos "${CMAKE_SOURCE_DIR}/icons/macos/${QLOGO_GUI}.icns")
    set_source_files_properties(${app_icon_macos} ${RESOURCE_FILES} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    qt_add_executable("${QLOGO_GUI}" MACOSX_BUNDLE
      ${GUI_SOURCES}
      ${app_icon_macos}
      ${RESOURCE_FILES}
      ${QLOGO_CSLS_FILES}
    )
else()
    qt_add_executable("${QLOGO_GUI}"
      ${GUI_SOURCES}
    )
endif()

target_include_directories("${QLOGO_GUI}" PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)

target_link_libraries("${QLOGO_GUI}" PRIVATE
    Qt6::Widgets
    Qt6::Svg
    Qt6::Sql
)

if(APPLE)
  set_target_properties("${QLOGO_GUI}" PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jason.qlogo.org
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    RESOURCE "${RESOURCE_FILES}"
  )
elseif(WIN32)
  set_target_properties("${QLOGO_GUI}" PROPERTIES
    WIN32_EXECUTABLE TRUE
  )
endif()

if(WIN32)
  get_target_property(_psychi_qmake_location Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(_psychi_qt_bin_dir "${_psychi_qmake_location}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${_psychi_qt_bin_dir}"
    REQUIRED
  )
  add_custom_command(TARGET "${QLOGO_GUI}" POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E env
      "PATH=${_psychi_qt_bin_dir};$ENV{PATH}"
      "${WINDEPLOYQT_EXECUTABLE}"
      "$<IF:$<CONFIG:Debug>,--debug,--release>"
      --no-translations
      "$<TARGET_FILE:${QLOGO_GUI}>"
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
      "${QLOGO_CSLS_SRC}"
      "$<TARGET_FILE_DIR:${QLOGO_GUI}>/csls"
    COMMENT "Deploying Qt runtime and CSLS files for ${QLOGO_GUI}"
    VERBATIM
  )
endif()

