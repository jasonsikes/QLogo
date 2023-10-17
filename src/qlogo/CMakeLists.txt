set(QLOGO_SOURCES
  controller/inputqueue.cpp
  controller/logocontroller.cpp
  controller/qlogocontroller.cpp
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
  executor/kernel_arithmetic.cpp
  executor/kernel_communication.cpp
  executor/kernel_controlstructures.cpp
  executor/kernel_datastructureprimitives.cpp
  executor/kernel_graphics.cpp
  executor/kernel_workspacemanagement.cpp
  logo_main.cpp
  misc/library.cpp
  misc/procedurehelper.cpp
  misc/stringconstants.cpp
  misc/turtle.cpp
  workspace/parser.cpp
  workspace/propertylists.cpp
  workspace/vars.cpp
  workspace/workspace.cpp
  
  ../include/sharedconstants.h
  ../include/inputqueue.h
  ../include/logocontroller.h
  ../include/qlogocontroller.h
  ../include/textstream.h
  ../include/datum.h
  ../include/datum_array.h
  ../include/datum_astnode.h
  ../include/datum_datump.h
  ../include/datum_iterator.h
  ../include/datum_list.h
  ../include/datum_word.h
  ../include/error.h
  ../include/kernel.h
  ../include/library.h
  ../include/procedurehelper.h
  ../include/stringconstants.h
  ../include/turtle.h
  ../include/parser.h
  ../include/propertylists.h
  ../include/vars.h
  ../include/workspace.h
)

qt_add_executable(qlogo
  MANUAL_FINALIZATION
  ${QLOGO_SOURCES}
)

target_include_directories(qlogo PUBLIC
  "${CMAKE_SOURCE_DIR}/src/include"
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

qt_finalize_executable(qlogo)

