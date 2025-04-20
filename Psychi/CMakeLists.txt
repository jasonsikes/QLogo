# Technically, the qlogo resource files belong to the qlogo binary,
# but on MacOS I want to put them into the QLOGO_GUI bundle.
set(RESOURCE_FILES
  ../../share/qlogo_help.db
  ../../share/qlogo_lib.db
)
set(GUI_SOURCES
  canvas.cpp
  console.cpp
  editorwindow.cpp
  mainwindow.cpp
  qlogo_main.cpp

  ../include/gui/canvas.h
  ../include/gui/console.h
  ../include/gui/editorwindow.h
  ../include/gui/mainwindow.h
  ../include/sharedconstants.h

  editorwindow.ui
  mainwindow.ui
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

set(MACOSX_BUNDLE_ICON_FILE "${QLOGO_GUI}.icns")

set(app_icon_macos "${CMAKE_SOURCE_DIR}/icons/${QLOGO_GUI}.icns")

set_source_files_properties(${app_icon_macos} PROPERTIES
    MACOSX_PACKAGE_LOCATION "Resources")


if (WIN32)
    file(COPY "${CMAKE_SOURCE_DIR}/icons/${QLOGO_GUI}.ico"
    DESTINATION  "${CMAKE_SOURCE_DIR}/resources")
    set(app_icon_resource_windows "${CMAKE_SOURCE_DIR}/resources/${QLOGO_GUI}.rc")
    qt_add_executable("${QLOGO_GUI}"
      ${GUI_SOURCES}
      ${app_icon_resource_windows}
    )
else()
    qt_add_executable("${QLOGO_GUI}" MACOSX_BUNDLE
      ${GUI_SOURCES}
      ${app_icon_macos}
    )
endif()



target_include_directories("${QLOGO_GUI}" PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)


target_link_libraries("${QLOGO_GUI}" PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Svg
    Qt${QT_VERSION_MAJOR}::Sql
)

set_target_properties("${QLOGO_GUI}" PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER jason.qlogo.org
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
  RESOURCE "${RESOURCE_FILES}"
)

