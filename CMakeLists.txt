cmake_minimum_required(VERSION 3.21)

# set the name of project
project(setaria)

# add the path of local cmake scripts
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# remember replace XXX to the proper name
include(Global_Options)

list(APPEND SETARIA_DEFINITIONS SETARIA_ROOT_PATH="${CMAKE_CURRENT_SOURCE_DIR}")

# add third party libraries
add_subdirectory(third_party)

# add the library
add_subdirectory(framework)

if(SETARIA_BUILD_APPS)
    # add applications
    add_subdirectory(apps)
endif()

if(SETARIA_BUILD_TESTS)
    # add tests
    add_subdirectory(tests)
endif()