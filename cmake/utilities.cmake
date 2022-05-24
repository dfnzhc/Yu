#*****************************************************************************
# Copyright 2020 NVIDIA Corporation. All rights reserved.
#*****************************************************************************
include_guard(GLOBAL)

#------------------------------------------------------------------------------------
# Find dependencies for GLSL files (#include ...)
# Call 'glslc -M' to find all dependencies of the file and return the list
# in GLSL_DEPENDENCY
#
function(get_glsl_dependecies _SRC _FLAGS)

    get_filename_component(FILE_NAME ${_SRC} NAME)
    get_filename_component(DIR_NAME ${_SRC} DIRECTORY)

    message(STATUS " - Find dependencies for ${FILE_NAME}")
    #message(STATUS "calling : ${GLSLC} ${_FLAGS} -M ${_SRC} OUTPUT_VARIABLE DEP RESULT_VARIABLE RES")
    separate_arguments(_FLAGS)
    execute_process(COMMAND ${GLSLC} ${_FLAGS} -M ${_SRC} OUTPUT_VARIABLE DEP RESULT_VARIABLE RES)
    if (RES EQUAL 0)
        # Removing "name.spv: "
        string(REGEX REPLACE "[^:]*: " "" DEP ${DEP})
        # Splitting each path with a ';' 
        string(REPLACE " ${DIR_NAME}" ";${DIR_NAME}" DEP ${DEP})
        set(GLSL_DEPENDENCY ${DEP} PARENT_SCOPE)
    endif ()
endfunction()


#------------------------------------------------------------------------------------
# Function to compile all GLSL source files to Spir-V
#
# SOURCE_FILES : All sources to compile
# HEADER_FILES : Dependencie header files
# DST : The destination directory (need to be absolute)
# VULKAN_TARGET : to define the vulkan target i.e vulkan1.2 (default vulkan1.1)
# HEADER ON: if ON, will generate headers instead of binary Spir-V files
# DEPENDENCY : ON|OFF will create the list of dependencies for the GLSL source file
# 
# compile_glsl(
#   SOURCES_FILES foo.vert foo.frag
#   DST ${CMAKE_CURRENT_SOURCE_DIR}/shaders
#   FLAGS -g0
# )

function(compile_glsl)
    set(oneValueArgs DST VULKAN_TARGET HEADER DEPENDENCY FLAGS)
    set(multiValueArgs SOURCE_FILES HEADER_FILES)
    cmake_parse_arguments(COMPILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)
    # Check if the GLSL compiler is present
    if (NOT GLSL_VALIDATOR)
        message(ERROR "Could not find GLSL_VALIDATOR to compile shaders")
        return()
    endif ()

    # By default use Vulkan 1.1
    if (NOT DEFINED COMPILE_VULKAN_TARGET)
        set(COMPILE_VULKAN_TARGET vulkan1.1)
    endif ()

    # If destination is not present, same as source
    if (NOT DEFINED COMPILE_DST)
        message(ERROR " --- DST not defined")
        return()
    endif ()

    # Make the output directory if needed
    file(MAKE_DIRECTORY ${COMPILE_DST})

    # If no flag set -g (debug)
    if (NOT DEFINED COMPILE_FLAGS)
        set(COMPILE_FLAGS -g)
    endif ()

    separate_arguments(_FLG UNIX_COMMAND ${COMPILE_FLAGS})

    # Compiling all GLSL sources
    foreach (GLSL_SRC ${COMPILE_SOURCE_FILES})
        message("SHADER: ${GLSL_SRC}")

        # Find the dependency files for the GLSL source
        # or use all headers as dependencies.
        if (COMPILE_DEPENDENCY)
            get_glsl_dependecies(${GLSL_SRC} ${COMPILE_FLAGS})
        else ()
            set(GLSL_DEPENDENCY ${HEADER_FILES})
        endif ()

        # Default compiler command, always adding debug information (Add and option to opt-out?)
        set(COMPILE_CMD ${_FLG} --target-env ${COMPILE_VULKAN_TARGET})

        # Compilation to headers need a variable name, the output will be a .h
        get_filename_component(FILE_NAME ${GLSL_SRC} NAME)
        if (COMPILE_HEADER)
            STRING(REPLACE "." "_" VAR_NAME ${FILE_NAME}) # Name of the variable in the header
            list(APPEND COMPILE_CMD --vn ${VAR_NAME})
            set(GLSL_OUT "${COMPILE_DST}/${FILE_NAME}.h")
        else ()
            set(GLSL_OUT "${COMPILE_DST}/${FILE_NAME}.spv")
            list(APPEND _SPVS ${GLSL_OUT})
        endif ()

        message("= ${_SPVS}")

        # Appending the output name and the file source
        list(APPEND COMPILE_CMD -o ${GLSL_OUT} -V ${GLSL_SRC})

        # The custom command is added to the build system, check for the presence of the output
        # but also for changes done in GLSL headers 
        add_custom_command(
                PRE_BUILD
                OUTPUT ${GLSL_OUT}
                COMMAND echo ${GLSL_VALIDATOR} ${COMPILE_CMD}
                COMMAND ${GLSL_VALIDATOR} ${COMPILE_CMD}
                MAIN_DEPENDENCY ${GLSL_SRC}
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                DEPENDS ${GLSL_DEPENDENCY}
        )
    endforeach ()

    # Setting OUT variables 
    set(GLSL_SOURCES ${COMPILE_SOURCE_FILES} PARENT_SCOPE)
    set(GLSL_HEADERS ${COMPILE_HEADER_FILES} PARENT_SCOPE)
    set(SPV_OUTPUT ${_SPVS} PARENT_SCOPE)

endfunction()


#------------------------------------------------------------------------------------
# Function to compile all GLSL files from a source to Spir-V
# The sources are all .vert, .frag, .r*  and the headers for the source are .glsl and .h
# This allows to modify one of the header and getting the sources recompiled.
#
# SRC : The directory source of the shaders
# DST : The destination directory (need to be absolute)
# VULKAN_TARGET : to define the vulkan target i.e vulkan1.2 (default vulkan1.1)
# HEADER ON: if present, will generate headers instead of binary Spir-V files
# DEPENDENCY : ON|OFF will create the list of dependencies for the GLSL source file 
# FLAGS : other glslValidator flags 
#
# compile_glsl_directory(
#    SRC "${CMAKE_CURRENT_SOURCE_DIR}/shaders" 
#    DST "${CMAKE_CURRENT_SOURCE_DIR}/autogen" 
#    VULKAN_TARGET "vulkan1.2"
#    HEADER ON
#    )
#
function(compile_glsl_directory)
    set(oneValueArgs SRC DST VULKAN_TARGET HEADER DEPENDENCY FLAGS)
    set(multiValueArgs)
    cmake_parse_arguments(COMPILE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    message("S ${COMPILE_SRC}")
    message("D ${COMPILE_DST}")

    # Collecting all source files
    file(GLOB GLSL_SOURCE_FILES
            "${COMPILE_SRC}/*.comp"     # Compute
            "${COMPILE_SRC}/*.frag"     # Fragment
            "${COMPILE_SRC}/*.geom"     # Geometry
            "${COMPILE_SRC}/*.mesh"     # Mesh
            "${COMPILE_SRC}/*.rahit"    # Ray any hit
            "${COMPILE_SRC}/*.rcall"    # Ray callable
            "${COMPILE_SRC}/*.rchit"    # Ray closest hit
            "${COMPILE_SRC}/*.rgen"     # Ray generation
            "${COMPILE_SRC}/*.rint"     # Ray intersection
            "${COMPILE_SRC}/*.rmiss"    # Ray miss
            "${COMPILE_SRC}/*.task"     # Task
            "${COMPILE_SRC}/*.tesc"     # Tessellation control
            "${COMPILE_SRC}/*.tese"     # Tessellation evaluation
            "${COMPILE_SRC}/*.vert"     # Vertex
            )

    # Collecting headers for dependencies
    file(GLOB GLSL_HEADER_FILES
            "${COMPILE_SRC}/*.glsl"     # Auto detect - used for header
            "${COMPILE_SRC}/*.h"
            )

    # By default use Vulkan 1.1
    if (NOT DEFINED COMPILE_VULKAN_TARGET)
        set(COMPILE_VULKAN_TARGET vulkan1.1)
    endif ()

    # If destination is not present, same as source
    if (NOT DEFINED COMPILE_DST)
        set(COMPILE_DST ${COMPILE_SRC})
    endif ()

    # If no flag set -g (debug)
    if (NOT DEFINED COMPILE_FLAGS)
        set(COMPILE_FLAGS -g)
    endif ()

    # Compiling all GLSL
    compile_glsl(SOURCE_FILES ${GLSL_SOURCE_FILES}
            HEADER_FILES ${GLSL_HEADER_FILES}
            DST ${COMPILE_DST}
            VULKAN_TARGET ${COMPILE_VULKAN_TARGET}
            HEADER ${COMPILE_HEADER}
            DEPENDENCY ${COMPILE_DEPENDENCY}
            FLAGS ${COMPILE_FLAGS}
            )

    # Setting OUT variables 
    set(GLSL_SOURCES ${GLSL_SOURCE_FILES} PARENT_SCOPE)
    set(GLSL_HEADERS ${GLSL_HEADER_FILES} PARENT_SCOPE)
    set(SPV_OUTPUT ${SPV_OUTPUT} PARENT_SCOPE) # propagate value set in compile_glsl
endfunction()

