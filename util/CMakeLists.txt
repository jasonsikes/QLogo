set(GEN_HELP_SOURCES
  gen_help.cpp
)

find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Sql)

qt_add_executable("${GEN_HELP_BIN}"
  ${GEN_HELP_SOURCES}
)

target_link_libraries("${GEN_HELP_BIN}" PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Sql
)

set_target_properties("${GEN_HELP_BIN}" PROPERTIES
  WIN32_EXECUTABLE TRUE
)


