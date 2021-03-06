﻿cmake_minimum_required(VERSION 3.21)

# set the name of project
project(Yu)

# add the path of local cmake scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# remember replace XXX to the proper name
include(Global_Options)
list(APPEND YU_DEFINITIONS YU_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

include(utilities)
#--------------------------------------------------------------------------------------------------
# Compiled shader to SPIR-V
compile_glsl_directory(
        SRC "${CMAKE_CURRENT_SOURCE_DIR}/data/shaders"
        DST "${CMAKE_CURRENT_SOURCE_DIR}/data/shaders/spv"
        VULKAN_TARGET "vulkan1.3"
)

add_custom_target(
        Shaders
        DEPENDS ${SPV_OUTPUT}
)

# add third party libraries
add_subdirectory(third_party)

# add the library
add_subdirectory(framework)

if (YU_BUILD_APPS)
    # add applications
    add_subdirectory(apps)
endif ()

set(YU_BUILD_TESTS OFF)
if (YU_BUILD_TESTS)
    # add tests
    add_subdirectory(tests)
endif ()
