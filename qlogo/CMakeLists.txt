set(QLOGO_SOURCES
  controller/inputqueue.cpp
  controller/logocontroller.cpp
  controller/logocontrollergui.cpp
  controller/textstream.cpp
  datum/datum.cpp
  datum/datum_array.cpp
  datum/datum_astnode.cpp
  datum/datum_datump.cpp
  datum/datum_iterator.cpp
  datum/datum_list.cpp
  datum/datum_word.cpp
  datum/error.cpp
  executor/kernel.cpp
  executor/arithmetic.cpp
  executor/communication.cpp
  executor/controlstructures.cpp
  executor/datastructureprimitives.cpp
  executor/graphics.cpp
  executor/workspacemanagement.cpp
  logo_main.cpp
  misc/help.cpp
  misc/library.cpp
  misc/procedurehelper.cpp
  misc/turtle.cpp
  parser/parser.cpp
  parser/runparser.cpp
  workspace/callframe.cpp
  workspace/procedures.cpp
  workspace/propertylists.cpp
  workspace/workspace.cpp

  ../include/sharedconstants.h
  ../include/controller/inputqueue.h
  ../include/controller/logocontroller.h
  ../include/controller/logocontrollergui.h
  ../include/controller/textstream.h
  ../include/datum.h
  ../include/astnode.h
  ../include/error.h
  ../include/help.h
  ../include/kernel.h
  ../include/primitives.h
  ../include/library.h
  ../include/procedurehelper.h
  ../include/turtle.h
  ../include/parser.h
  ../include/runparser.h
  ../include/workspace/callframe.h
  ../include/workspace/primitivetable.h
  ../include/workspace/procedures.h
  ../include/workspace/propertylists.h
  ../include/workspace/workspace.h
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

qt_add_executable("${QLOGO_BIN}"
  ${QLOGO_SOURCES}
)

target_include_directories("${QLOGO_BIN}" PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)

add_definitions(-DLOGOVERSION="${PROJECT_VERSION}")

if(APPLE)
  add_definitions(-DLOGOPLATFORM="OSX")
elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
  add_definitions(-DLOGOPLATFORM="WINDOWS")
else()
  add_definitions(-DLOGOPLATFORM="UNIX")
endif()

target_link_libraries("${QLOGO_BIN}" PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
)

set_target_properties("${QLOGO_BIN}" PROPERTIES
  WIN32_EXECUTABLE TRUE
)


