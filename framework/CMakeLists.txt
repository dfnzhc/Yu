cmake_minimum_required(VERSION 3.21)

# set the name of project
project(framework LANGUAGES C CXX)

set(COMMON_FILES
        # header files
        common/logger.hpp
        )

# set the group of the source files
source_group("common\\" FILES ${COMMON_FILES})


set(PROJECT_FILES
        ${COMMON_FILES}
        )

set_source_files_properties(${PROJECT_FILES} PROPERTIES LANGUAGE CXX)

add_library(${PROJECT_NAME} STATIC ${PROJECT_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${SETARIA_DEFINITIONS})
target_precompile_headers(${PROJECT_NAME} PRIVATE pch.hpp)

if(${SETARIA_WARNING_AS_ERROR})
    message(STATUS "Warnings as Errors Enabled")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    endif()
endif()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} 
        PUBLIC
        spdlog
        )
