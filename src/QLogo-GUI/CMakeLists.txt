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

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_add_executable(QLogo-GUI
  MANUAL_FINALIZATION
  ${GUI_SOURCES}
)

target_include_directories(QLogo-GUI PUBLIC
  "${CMAKE_SOURCE_DIR}/src/include"
)

target_link_libraries(QLogo-GUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)

set_target_properties(QLogo-GUI PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER jason.qlogo.org
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(QLogo-GUI)
