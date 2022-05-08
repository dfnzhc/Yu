cmake_minimum_required(VERSION 3.21)
project(CGT_Test LANGUAGES CXX CUDA)

find_package(Catch2 CONFIG REQUIRED)

# These examples use the standard separate compilation
set(SOURCES_IDIOMATIC_EXAMPLES
        test.cpp
        )

string(REPLACE ".cpp" "" BASENAMES_IDIOMATIC_EXAMPLES "${SOURCES_IDIOMATIC_EXAMPLES}")
set(TARGETS_IDIOMATIC_EXAMPLES ${BASENAMES_IDIOMATIC_EXAMPLES})


foreach (name ${TARGETS_IDIOMATIC_EXAMPLES})
    add_executable(${name} ${name}.cpp)
endforeach ()

set(ALL_EXAMPLE_TARGETS
        ${TARGETS_IDIOMATIC_EXAMPLES}
        )

foreach (name ${ALL_EXAMPLE_TARGETS})
    target_include_directories(${name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${name} PRIVATE Catch2::Catch2 THE_LIB_NAME)
    set_property(TARGET ${name} PROPERTY CXX_STANDARD 23)
endforeach ()