set(QLOGO_SOURCES
  compiler/compiler.cpp
  compiler/compiler_arithmetic.cpp
  compiler/compiler_communication.cpp
  compiler/compiler_datastructureprimitives.cpp
  compiler/compiler_workspacemanagement.cpp
  compiler/compiler_controlstructures.cpp
  compiler/compiler_graphics.cpp
  compiler/compiler_specialvariables.cpp
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
  datum/datum_flowcontrol.cpp
  kernel.cpp
  logo_main.cpp
  misc/library.cpp
  misc/turtle.cpp
  misc/exports.cpp
  parser/parser.cpp
  parser/runparser.cpp
  workspace/callframe.cpp
  workspace/procedures.cpp
  workspace/propertylists.cpp

  ../include/sharedconstants.h
  ../include/compiler.h
  ../include/compiler_private.h
  ../include/controller/inputqueue.h
  ../include/controller/logocontroller.h
  ../include/controller/logocontrollergui.h
  ../include/controller/textstream.h
  ../include/datum.h
  ../include/astnode.h
  ../include/flowcontrol.h
  ../include/kernel.h
  ../include/primitives.h
  ../include/library.h
  ../include/turtle.h
  ../include/parser.h
  ../include/runparser.h
  ../include/workspace/callframe.h
  ../include/workspace/primitivetable.h
  ../include/workspace/procedures.h
  ../include/workspace/propertylists.h
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})

qt_add_executable("${QLOGO_BIN}"
  ${QLOGO_SOURCES}
)

target_include_directories("${QLOGO_BIN}" PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
)

add_definitions(${LLVM_DEFINITIONS})

add_definitions(-DLOGOVERSION="${PROJECT_VERSION}")

if(APPLE)
  add_definitions(-DLOGOPLATFORM="OSX")
elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
  add_definitions(-DLOGOPLATFORM="WINDOWS")
else()
  add_definitions(-DLOGOPLATFORM="UNIX")
endif()

llvm_config("${QLOGO_BIN}" USE_SHARED support core orcjit native irreader)

if(APPLE)
target_link_libraries("${QLOGO_BIN}" PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Sql
    -Xlinker
    ${llvm_libs}
)
else()
  target_link_libraries("${QLOGO_BIN}" PRIVATE
      Qt${QT_VERSION_MAJOR}::Core
      Qt${QT_VERSION_MAJOR}::Widgets
      Qt${QT_VERSION_MAJOR}::Sql
      -Xlinker --export-dynamic
      ${llvm_libs}
  )
endif()

set_target_properties("${QLOGO_BIN}" PROPERTIES
  WIN32_EXECUTABLE TRUE
)


