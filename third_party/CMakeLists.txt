cmake_minimum_required(VERSION 3.21)

project(third_party LANGUAGES C CXX)

set(SAN_BUILD_TESTS OFF)
add_subdirectory(San)
add_library(san INTERFACE)
target_link_libraries(san INTERFACE San::San)
target_include_directories(san INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/San/source")
set_target_properties(san PROPERTIES FOLDER "ThirdParty")

# glm
find_package(glm CONFIG REQUIRED)
add_library(glm INTERFACE)
target_link_libraries(glm INTERFACE glm::glm)
set_target_properties(glm PROPERTIES FOLDER "ThirdParty")

# imgui [docking]
find_package(imgui CONFIG REQUIRED)
add_library(imgui INTERFACE)
target_link_libraries(imgui INTERFACE imgui::imgui)
set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")

# vma
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
add_library(vma INTERFACE)
target_link_libraries(vma INTERFACE unofficial::vulkan-memory-allocator::vulkan-memory-allocator)
set_target_properties(vma PROPERTIES FOLDER "ThirdParty")

# stb
find_path(STB_INCLUDE_DIRS "stb.h")
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${STB_INCLUDE_DIRS})

