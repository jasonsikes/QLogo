set(QLOGO_SOURCES
  compiler/compiler.cpp
  compiler/compiler_arithmetic.cpp
  compiler/compiler_communication.cpp
  compiler/compiler_datastructureprimitives.cpp
  compiler/compiler_workspacemanagement.cpp
  compiler/compiler_controlstructures.cpp
  compiler/compiler_graphics.cpp
  compiler/compiler_specialvariables.cpp
  interface/inputqueue.cpp
  interface/logointerface.cpp
  interface/logointerfacegui.cpp
  interface/streammanager.cpp
  interface/textstream.cpp
  datum/datum.cpp
  datum/datum_array.cpp
  datum/datum_astnode.cpp
  datum/datum_datump.cpp
  datum/datum_iterator.cpp
  datum/datum_list.cpp
  datum/datum_word.cpp
  datum/datum_flowcontrol.cpp
  workspace/kernel.cpp
  logo_main.cpp
  workspace/library.cpp
  workspace/turtle.cpp
  workspace/evaluator.cpp
  workspace/exports.cpp
  workspace/visited.cpp
  parser/treeifyer.cpp
  parser/runparser.cpp
  parser/cmd_strings.cpp
  parser/op_strings.cpp
  workspace/callframe.cpp
  workspace/procedures.cpp
  workspace/propertylists.cpp

  ../include/sharedconstants.h
  ../include/cmd_strings.h
  ../include/compiler.h
  ../include/compiler_internal.h
  ../include/compiler_types.h
  ../include/interface/inputqueue.h
  ../include/interface/pipeio.h
  ../include/interface/logointerface.h
  ../include/interface/logointerfacegui.h
  ../include/interface/streammanager.h
  ../include/interface/textstream.h
  ../include/datum_core.h
  ../include/datum_ptr.h
  ../include/datum_types.h
  ../include/astnode.h
  ../include/flowcontrol.h
  ../include/workspace/kernel.h
  ../include/primitives.h
  ../include/workspace/evaluator.h
  ../include/workspace/exports.h
  ../include/workspace/library.h
  ../include/workspace/turtle.h
  ../include/treeifyer.h
  ../include/runparser.h
  ../include/op_strings.h
  ../include/workspace/visited.h
  ../include/workspace/callframe.h
  ../include/workspace/primitivetable.h
  ../include/workspace/procedures.h
  ../include/workspace/propertylists.h
)

find_package(Qt6 REQUIRED COMPONENTS Sql)
find_package(LLVM REQUIRED CONFIG)

if(MSVC)
  include("${CMAKE_SOURCE_DIR}/cmake/FixLLVMWindowsDIA.cmake")
  fix_llvm_windows_dia_sdk()
endif()

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

target_compile_definitions("${QLOGO_BIN}" PRIVATE QT_FORCE_ASSERTS)

if(APPLE)
  target_compile_definitions("${QLOGO_BIN}" PRIVATE "LOGOPLATFORM=\"OSX\"")
elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
  target_compile_definitions("${QLOGO_BIN}" PRIVATE "LOGOPLATFORM=\"WINDOWS\"")
else()
  target_compile_definitions("${QLOGO_BIN}" PRIVATE "LOGOPLATFORM=\"UNIX\"")
endif()

# LLVM shared libraries (libLLVM) are not supported with the Microsoft
# toolchain; USE_SHARED would try to link LLVM.lib, which is not built.
if(MSVC)
  llvm_config("${QLOGO_BIN}" support core orcjit native irreader)
else()
  llvm_config("${QLOGO_BIN}" USE_SHARED support core orcjit native irreader)
endif()

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

# Copy Qt runtime DLLs/plugins next to qlogo.exe so it can be run and
# tested from the build tree without manually adjusting PATH.
if(WIN32)
  get_target_property(_qlogo_qmake_location Qt6::qmake IMPORTED_LOCATION)
  get_filename_component(_qlogo_qt_bin_dir "${_qlogo_qmake_location}" DIRECTORY)
  find_program(WINDEPLOYQT_EXECUTABLE
    NAMES windeployqt windeployqt6
    HINTS "${_qlogo_qt_bin_dir}"
    REQUIRED
  )
  add_custom_command(TARGET "${QLOGO_BIN}" POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E env
      "PATH=${_qlogo_qt_bin_dir};$ENV{PATH}"
      "${WINDEPLOYQT_EXECUTABLE}"
      "$<IF:$<CONFIG:Debug>,--debug,--release>"
      --no-translations
      "$<TARGET_FILE:${QLOGO_BIN}>"
    COMMENT "Deploying Qt runtime for ${QLOGO_BIN}"
    VERBATIM
  )
endif()

