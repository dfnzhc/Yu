cmake_minimum_required(VERSION 3.21)

# set the name of project
project(framework LANGUAGES C CXX)

# vulkan
find_package(Vulkan REQUIRED)

set(COMMON_FILES
        # header files
        pch.hpp

        common/common.hpp common/helpers.hpp common/logger.hpp)

set(RHI_VULKAN_FILES
        # header files
        RHI/vulkan/vk_error.hpp
        
        # source files
        RHI/vulkan/vk_error.cpp
        RHI/vulkan/vk_appbase.cpp RHI/vulkan/vk_appbase.hpp RHI/vulkan/vk_debug.cpp RHI/vulkan/vk_debug.hpp)

# set the group of the source files
source_group("common\\"     FILES ${COMMON_FILES})
source_group("RHI\\vulkan"  FILES ${RHI_VULKAN_FILES})

set(PROJECT_FILES
        ${COMMON_FILES}
        ${RHI_VULKAN_FILES}
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

target_compile_definitions(${PROJECT_NAME} PUBLIC VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME} 
        PUBLIC
        Vulkan::Vulkan
        yuan
        glm
        )
