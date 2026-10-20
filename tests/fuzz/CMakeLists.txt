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

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/runparser_corpus"
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}"
)
