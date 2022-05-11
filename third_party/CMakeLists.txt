cmake_minimum_required(VERSION 3.21)

project(third_party LANGUAGES C CXX)

# spdlog
set(YUAN_BUILD_TESTS OFF)
add_subdirectory(yuan)
add_library(yuan INTERFACE)
target_link_libraries(yuan INTERFACE Yuan::Yuan)
target_include_directories(yuan INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/yuan/source")
set_target_properties(yuan PROPERTIES FOLDER "ThirdParty")

# vma 
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
add_library(vma INTERFACE)
target_link_libraries(vma INTERFACE unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

# volk
find_package(volk CONFIG REQUIRED)
add_library(volk INTERFACE)
target_link_libraries(volk INTERFACE volk::volk volk::volk_headers)