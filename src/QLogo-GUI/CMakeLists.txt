# Technically, the qlogo resource files belong to the qlogo binary,
# but on MacOS I want to put them into the QLogo-GUI bundle.
set(RESOURCE_FILES
  ../../share/qlogo_help.db
)
set(GUI_SOURCES
  canvas.cpp
  console.cpp
  editorwindow.cpp
  mainwindow.cpp
  qlogo_main.cpp

  ../include/canvas.h
  ../include/console.h
  ../include/editorwindow.h
  ../include/mainwindow.h
  ../include/sharedconstants.h

  editorwindow.ui
  mainwindow.ui
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

qt_add_executable("${QLOGO_GUI}"
  ${GUI_SOURCES}
)

target_include_directories("${QLOGO_GUI}" PUBLIC
  "${CMAKE_SOURCE_DIR}/src/include"
)


target_link_libraries("${QLOGO_GUI}" PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
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

