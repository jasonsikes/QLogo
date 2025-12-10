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

find_package(Qt6 REQUIRED COMPONENTS Sql)
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

set(LLVM_DEFINITIONS_CLEANED)
foreach(def ${LLVM_DEFINITIONS})
  string(REGEX REPLACE "^-D" "" cleaned_def "${def}")
  list(APPEND LLVM_DEFINITIONS_CLEANED "${cleaned_def}")
endforeach()

qt_add_executable("${QLOGO_BIN}"
  ${QLOGO_SOURCES}
)

target_include_directories("${QLOGO_BIN}" PUBLIC
  "${CMAKE_SOURCE_DIR}/include"
  ${LLVM_INCLUDE_DIRS}
)

# For string literal macros, use target_compile_definitions() which correctly
# handles quoting for preprocessor definitions. The escaped quotes in the
# definition value will be preserved and passed to the compiler as -DLOGOVERSION="0.961"
target_compile_definitions("${QLOGO_BIN}" PRIVATE
  ${LLVM_DEFINITIONS_CLEANED}
  "LOGOVERSION=\"${PROJECT_VERSION}\""
)

if(APPLE)
  target_compile_definitions("${QLOGO_BIN}" PRIVATE "LOGOPLATFORM=\"OSX\"")
elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
  target_compile_definitions("${QLOGO_BIN}" PRIVATE "LOGOPLATFORM=\"WINDOWS\"")
else()
  target_compile_definitions("${QLOGO_BIN}" PRIVATE "LOGOPLATFORM=\"UNIX\"")
endif()

llvm_config("${QLOGO_BIN}" USE_SHARED support core orcjit native irreader)

if(APPLE)
  target_link_libraries("${QLOGO_BIN}" PRIVATE
      Qt6::Core
      Qt6::Widgets
      Qt6::Sql
      -Xlinker
      ${llvm_libs}
  )
elseif(WIN32)
  target_link_libraries("${QLOGO_BIN}" PRIVATE
      Qt6::Core
      Qt6::Widgets
      Qt6::Sql
      ${llvm_libs}
  )
else()
  target_link_libraries("${QLOGO_BIN}" PRIVATE
      Qt6::Core
      Qt6::Widgets
      Qt6::Sql
      -Xlinker --export-dynamic
      ${llvm_libs}
  )
endif()

if(WIN32)
  set_target_properties("${QLOGO_BIN}" PROPERTIES
    WIN32_EXECUTABLE TRUE
  )
endif()


