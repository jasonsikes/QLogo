cmake_minimum_required(VERSION 3.5)

project(QLogo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(LOGO_VERSION 0.93)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Widgets OpenGLWidgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets OpenGLWidgets)

set(QLOGO_SOURCES qlogo_main.cpp mainwindow.cpp canvas.cpp console.cpp editorwindow.cpp
    mainwindow.h canvas.h console.h constants.h editorwindow.h
    mainwindow.ui editorwindow.ui)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(QLogo
        MANUAL_FINALIZATION
        ${QLOGO_SOURCES}
    )
else()
    add_executable(QLogo
        ${QLOGO_SOURCES}
    )
endif()

target_link_libraries(QLogo PRIVATE Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGLWidgets)

set(LOGO_SOURCES datum.cpp parser.cpp turtle.cpp vars.cpp kernel.cpp propertylists.cpp kernel_datastructureprimitives.cpp
    kernel_communication.cpp kernel_arithmetic.cpp kernel_graphics.cpp kernel_workspacemanagement.cpp workspace.cpp
    procedurehelper.cpp help.cpp kernel_controlstructures.cpp error.cpp library.cpp datum_word.cpp datum_astnode.cpp
    datum_list.cpp datum_array.cpp datum_datump.cpp datum_iterator.cpp logo_main.cpp qlogocontroller.cpp logocontroller.cpp
    inputqueue.cpp
    datum.h parser.h turtle.h vars.h kernel.h propertylists.h workspace.h procedurehelper.h help.h error.h constants.h
    qlogocontroller.h logocontroller.h library.h inputqueue.h)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(QLogo-cli
        MANUAL_FINALIZATION
        ${LOGO_SOURCES}
    )
else()
    add_executable(QLogo-cli
        ${LOGO_SOURCES}
    )
endif()
add_definitions(-DLOGOVERSION="${LOGO_VERSION}")

if(APPLE)
  add_definitions(-DLOGOPLATFORM="OSX")
elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
  add_definitions(-DLOGOPLATFORM="WINDOWS")
else()
  add_definitions(-DLOGOPLATFORM="UNIX")
endif()

target_link_libraries(QLogo-cli PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(QLogo-cli PROPERTIES
    WIN32_EXECUTABLE TRUE
)

set_target_properties(QLogo PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER jason.psyches.org
    MACOSX_BUNDLE_BUNDLE_VERSION ${LOGO_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(QLogo)
    qt_finalize_executable(QLogo-cli)
endif()

install(TARGETS QLogo QLogo-cli RUNTIME DESTINATION . BUNDLE DESTINATION ${CMAKE_INSTALL_BINDIR})
