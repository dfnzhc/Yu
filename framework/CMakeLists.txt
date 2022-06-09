cmake_minimum_required(VERSION 3.21)

# set the name of project
project(framework LANGUAGES C CXX)

# vulkan
find_package(Vulkan REQUIRED)

set(COMMON_FILES
        # header files
        pch.hpp
        common/common.hpp

        # source files
        )

set(VULKAN_FILES
        # header files
        RHI/vulkan/initializers.hpp
        RHI/vulkan/instance_properties.hpp
        RHI/vulkan/instance.hpp 
        RHI/vulkan/error.hpp 
        RHI/vulkan/ext_debug.hpp

        # source files
        RHI/vulkan/instance_properties.cpp 
        RHI/vulkan/instance.cpp
        RHI/vulkan/error.cpp 
        RHI/vulkan/ext_debug.cpp
        RHI/vulkan/device.cpp RHI/vulkan/device.hpp RHI/vulkan/device_properties.cpp RHI/vulkan/device_properties.hpp RHI/vulkan/properties.hpp RHI/vulkan/properties.cpp RHI/vulkan/vulkan_utils.hpp RHI/vulkan/ext_float.cpp RHI/vulkan/ext_float.hpp RHI/vulkan/ext_hdr.cpp RHI/vulkan/ext_hdr.hpp RHI/vulkan/ext_raytracing.cpp RHI/vulkan/ext_raytracing.hpp RHI/vulkan/swap_chain.cpp RHI/vulkan/swap_chain.hpp RHI/vulkan/pipeline.cpp RHI/vulkan/pipeline.hpp RHI/vulkan/vulkan_utils.cpp RHI/vulkan/command.cpp RHI/vulkan/command.hpp RHI/vulkan/appbase.cpp RHI/vulkan/appbase.hpp RHI/vulkan/renderer.cpp RHI/vulkan/renderer.hpp)

# set the group of the source files
source_group("common\\" FILES ${COMMON_FILES})
source_group("RHI\\vulkan\\" FILES ${VULKAN_FILES})

set(PROJECT_FILES
        ${COMMON_FILES}
        ${VULKAN_FILES}
        )

set_source_files_properties(${PROJECT_FILES} PROPERTIES LANGUAGE CXX)

add_library(${PROJECT_NAME} STATIC ${PROJECT_FILES})

target_compile_definitions(${PROJECT_NAME} PRIVATE ${YU_DEFINITIONS})
target_precompile_headers(${PROJECT_NAME} PRIVATE pch.hpp)

if (${YU_WARNING_AS_ERROR})
    message(STATUS "Warnings as Errors Enabled")
    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        target_compile_options(${PROJECT_NAME} PRIVATE -Werror)
    elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /WX)
    endif ()
endif ()

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(${PROJECT_NAME}
        PUBLIC
        Vulkan::Vulkan
        san
        glm
        imgui
        vma
        )
