﻿cmake_minimum_required(VERSION 3.21)

# set the name of project
project(framework LANGUAGES C CXX)

# vulkan
find_package(Vulkan REQUIRED)

set(COMMON_FILES
        # header files
        pch.hpp
        common/common.hpp
        common/buffer_ring.hpp 
        common/camera.hpp 
        common/mouse_tracker.hpp 
        common/stb_inc.hpp
        common/Bitmap.hpp
        common/math_utils.hpp
        common/imgui_impl_glfw.h

        # source files
        common/mouse_tracker.cpp 
        common/stb_inc.cpp 
        common/Bitmap.cpp
        common/imgui_impl_glfw.cpp
        )

set(VULKAN_FILES
        # header files
        RHI/vulkan/initializers.hpp
        RHI/vulkan/instance_properties.hpp
        RHI/vulkan/instance.hpp 
        RHI/vulkan/error.hpp 
        RHI/vulkan/ext_debug.hpp
        RHI/vulkan/device.hpp 
        RHI/vulkan/device_properties.hpp 
        RHI/vulkan/properties.hpp 
        RHI/vulkan/vulkan_utils.hpp 
        RHI/vulkan/ext_float.hpp 
        RHI/vulkan/ext_hdr.hpp 
        RHI/vulkan/ext_raytracing.hpp 
        RHI/vulkan/swap_chain.hpp 
        RHI/vulkan/pipeline.hpp
        RHI/vulkan/commands.hpp 
        RHI/vulkan/appbase.hpp 
        RHI/vulkan/renderer.hpp 
        RHI/vulkan/dynamic_buffer.hpp 
        RHI/vulkan/descriptor_pool.hpp 
        RHI/vulkan/static_buffer.hpp 
        RHI/vulkan/pipeline_builder.hpp 
        RHI/vulkan/texture.hpp 
        RHI/vulkan/upload_heap.hpp 
        RHI/vulkan/gbuffer.hpp 
        RHI/vulkan/imgui.hpp
        RHI/vulkan/buffer.hpp
        RHI/vulkan/imgui_impl_vulkan.h
        RHI/vulkan/model_obj.hpp

        # source files
        RHI/vulkan/instance_properties.cpp 
        RHI/vulkan/instance.cpp
        RHI/vulkan/error.cpp 
        RHI/vulkan/ext_debug.cpp
        RHI/vulkan/device.cpp 
        RHI/vulkan/device_properties.cpp 
        RHI/vulkan/properties.cpp 
        RHI/vulkan/ext_float.cpp 
        RHI/vulkan/ext_hdr.cpp 
        RHI/vulkan/ext_raytracing.cpp 
        RHI/vulkan/swap_chain.cpp 
        RHI/vulkan/pipeline.cpp 
        RHI/vulkan/vulkan_utils.cpp
        RHI/vulkan/commands.cpp 
        RHI/vulkan/appbase.cpp 
        RHI/vulkan/renderer.cpp 
        RHI/vulkan/dynamic_buffer.cpp 
        RHI/vulkan/descriptor_pool.cpp 
        RHI/vulkan/static_buffer.cpp 
        RHI/vulkan/pipeline_builder.cpp 
        RHI/vulkan/texture.cpp 
        RHI/vulkan/upload_heap.cpp 
        RHI/vulkan/gbuffer.cpp 
        RHI/vulkan/imgui.cpp
        RHI/vulkan/imgui_impl_vulkan.cpp
        RHI/vulkan/buffer.cpp
        RHI/vulkan/model_obj.cpp
        RHI/vulkan/gpu_time.cpp RHI/vulkan/gpu_time.hpp)

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
        stb
        tinyobj
        tinygltf
        )
