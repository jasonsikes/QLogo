if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR
        "QLOGO_ENABLE_FUZZING requires LLVM Clang with libFuzzer "
        "(CMAKE_CXX_COMPILER_ID is ${CMAKE_CXX_COMPILER_ID}). "
        "On macOS, configure with the MacPorts/Homebrew clang, not AppleClang.")
endif()

add_executable(fuzz_runparser
    fuzz_runparser.cpp
    fuzz_runparser_support.cpp
    "${CMAKE_SOURCE_DIR}/qlogo/parser/runparser.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/parser/op_strings.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/datum/datum.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/datum/datum_word.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/datum/datum_list.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/datum/datum_datump.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/datum/datum_iterator.cpp"
    "${CMAKE_SOURCE_DIR}/qlogo/workspace/visited.cpp"
)

set_target_properties(fuzz_runparser PROPERTIES
    AUTOMOC OFF
    AUTOUIC OFF
    AUTORCC OFF
    WIN32_EXECUTABLE FALSE
)

get_target_property(_qlogo_includes qlogo INCLUDE_DIRECTORIES)
target_include_directories(fuzz_runparser PRIVATE
    ${_qlogo_includes}
    "${CMAKE_SOURCE_DIR}/include"
)

get_target_property(_qlogo_defines qlogo COMPILE_DEFINITIONS)
if(_qlogo_defines)
    target_compile_definitions(fuzz_runparser PRIVATE ${_qlogo_defines})
endif()

# datum_datump.cpp includes LLVM headers via ASTNode. This target does not
# link LLVM, so skip the ABI-breaking-checks sentinel.
target_compile_definitions(fuzz_runparser PRIVATE LLVM_DISABLE_ABI_BREAKING_CHECKS_ENFORCING=1)

target_link_libraries(fuzz_runparser PRIVATE Qt6::Core Qt6::Gui)

target_compile_options(fuzz_runparser PRIVATE
    -fsanitize=fuzzer,address,undefined
    -fno-omit-frame-pointer
)
target_link_options(fuzz_runparser PRIVATE
    -fsanitize=fuzzer,address,undefined
)

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(_corpus_dir "${CMAKE_CURRENT_BINARY_DIR}/runparser_corpus")
set(_corpus_stamp "${CMAKE_CURRENT_BINARY_DIR}/runparser_corpus.stamp")

add_custom_command(
    OUTPUT "${_corpus_stamp}"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/split_seeds.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/seeds.txt"
        "${_corpus_dir}"
        "${_corpus_stamp}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/seeds.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/split_seeds.py"
    COMMENT "Splitting seeds.txt into runparser_corpus"
    VERBATIM
)
add_custom_target(fuzz_runparser_corpus DEPENDS "${_corpus_stamp}")
add_dependencies(fuzz_runparser fuzz_runparser_corpus)

# Treeifier fuzzer: link the interpreter (except logo_main) the same way qlogo does.
get_target_property(_qlogo_sources qlogo SOURCES)
get_target_property(_qlogo_src_dir qlogo SOURCE_DIR)
set(_fuzz_treeifier_sources "${CMAKE_CURRENT_SOURCE_DIR}/fuzz_treeifier.cpp")
foreach(_src ${_qlogo_sources})
    if(NOT _src MATCHES "logo_main\\.cpp$" AND NOT _src MATCHES "_autogen" AND NOT _src MATCHES "mocs_")
        if(NOT IS_ABSOLUTE "${_src}")
            set(_src "${_qlogo_src_dir}/${_src}")
        endif()
        list(APPEND _fuzz_treeifier_sources "${_src}")
    endif()
endforeach()

add_executable(fuzz_treeifier ${_fuzz_treeifier_sources})

set_target_properties(fuzz_treeifier PROPERTIES
    AUTOMOC ON
    AUTOUIC OFF
    AUTORCC OFF
    WIN32_EXECUTABLE FALSE
)

target_include_directories(fuzz_treeifier PRIVATE
    ${_qlogo_includes}
    "${CMAKE_SOURCE_DIR}/include"
)

if(_qlogo_defines)
    target_compile_definitions(fuzz_treeifier PRIVATE ${_qlogo_defines})
endif()

target_compile_definitions(fuzz_treeifier PRIVATE
    "QLOGO_FUZZ_LIBRARY_DB=\"${QLOGO_LIB_DB}\""
)
add_dependencies(fuzz_treeifier qlogo_library_db)

find_package(LLVM REQUIRED CONFIG)
if(MSVC)
    llvm_config(fuzz_treeifier support core orcjit orcdebugging native irreader)
else()
    llvm_config(fuzz_treeifier USE_SHARED support core orcjit orcdebugging native irreader)
endif()

if(APPLE)
    target_link_libraries(fuzz_treeifier PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        -Xlinker
        ${llvm_libs}
    )
elseif(WIN32)
    target_link_libraries(fuzz_treeifier PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        ${llvm_libs}
    )
else()
    target_link_libraries(fuzz_treeifier PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Sql
        -Xlinker --export-dynamic
        ${llvm_libs}
    )
endif()

if(NOT WIN32)
    set(CURSES_NEED_NCURSES TRUE)
    set(CURSES_NEED_WIDE TRUE)
    find_package(Curses)
    if(NOT CURSES_FOUND)
        set(CURSES_NEED_WIDE FALSE)
        find_package(Curses REQUIRED)
    endif()
    target_include_directories(fuzz_treeifier PRIVATE ${CURSES_INCLUDE_DIRS})
    target_link_libraries(fuzz_treeifier PRIVATE ${CURSES_LIBRARIES})
    foreach(_src ${_fuzz_treeifier_sources})
        if(_src MATCHES "logointerfacecurses\\.cpp$")
            set_property(SOURCE "${_src}" APPEND PROPERTY
                COMPILE_DEFINITIONS NCURSES_WIDECHAR=1
            )
        endif()
    endforeach()
endif()

target_compile_options(fuzz_treeifier PRIVATE
    -fsanitize=fuzzer,address,undefined
    -fno-omit-frame-pointer
)
target_link_options(fuzz_treeifier PRIVATE
    -fsanitize=fuzzer,address,undefined
)

set(_treeifier_corpus_dir "${CMAKE_CURRENT_BINARY_DIR}/treeifier_corpus")
set(_treeifier_corpus_stamp "${CMAKE_CURRENT_BINARY_DIR}/treeifier_corpus.stamp")

add_custom_command(
    OUTPUT "${_treeifier_corpus_stamp}"
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/split_seeds.py"
        "${CMAKE_CURRENT_SOURCE_DIR}/treeifier_seeds.txt"
        "${_treeifier_corpus_dir}"
        "${_treeifier_corpus_stamp}"
    DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/treeifier_seeds.txt"
        "${CMAKE_CURRENT_SOURCE_DIR}/split_seeds.py"
    COMMENT "Splitting treeifier_seeds.txt into treeifier_corpus"
    VERBATIM
)
add_custom_target(fuzz_treeifier_corpus DEPENDS "${_treeifier_corpus_stamp}")
add_dependencies(fuzz_treeifier fuzz_treeifier_corpus)
