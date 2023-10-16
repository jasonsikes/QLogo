cmake_minimum_required(VERSION 3.5)

project(QLogo VERSION 0.931 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(GUI_SOURCES qlogo_main.cpp mainwindow.cpp canvas.cpp console.cpp editorwindow.cpp
  mainwindow.h canvas.h console.h sharedconstants.h editorwindow.h
  mainwindow.ui editorwindow.ui)

qt_add_executable(QLogo-GUI
  MANUAL_FINALIZATION
  ${GUI_SOURCES}
)

target_link_libraries(QLogo-GUI PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)

set(QLOGO_SOURCES datum.cpp parser.cpp turtle.cpp vars.cpp kernel.cpp propertylists.cpp kernel_datastructureprimitives.cpp
  kernel_communication.cpp kernel_arithmetic.cpp kernel_graphics.cpp kernel_workspacemanagement.cpp workspace.cpp
  procedurehelper.cpp kernel_controlstructures.cpp error.cpp library.cpp datum_word.cpp datum_astnode.cpp
  datum_list.cpp datum_array.cpp datum_datump.cpp datum_iterator.cpp logo_main.cpp qlogocontroller.cpp logocontroller.cpp
  inputqueue.cpp stringconstants.cpp textstream.cpp
  datum.h parser.h turtle.h vars.h kernel.h propertylists.h workspace.h procedurehelper.h error.h sharedconstants.h
  qlogocontroller.h logocontroller.h library.h inputqueue.h datum_word.h datum_astnode.h
  datum_list.h datum_array.h datum_datump.h datum_iterator.h stringconstants.h textstream.h)

qt_add_executable(qlogo
  MANUAL_FINALIZATION
  ${QLOGO_SOURCES}
)

add_definitions(-DLOGOVERSION="${PROJECT_VERSION}")

if(APPLE)
  add_definitions(-DLOGOPLATFORM="OSX")
elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
  add_definitions(-DLOGOPLATFORM="WINDOWS")
else()
  add_definitions(-DLOGOPLATFORM="UNIX")
endif()

target_link_libraries(qlogo PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(qlogo PROPERTIES
  WIN32_EXECUTABLE TRUE
)

set_target_properties(QLogo-GUI PROPERTIES
  MACOSX_BUNDLE_GUI_IDENTIFIER jason.qlogo.org
  MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
  MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
  MACOSX_BUNDLE TRUE
  WIN32_EXECUTABLE TRUE
)

qt_finalize_executable(QLogo-GUI)
qt_finalize_executable(qlogo)

install(TARGETS QLogo-GUI qlogo RUNTIME DESTINATION . BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR})
