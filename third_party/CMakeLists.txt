cmake_minimum_required(VERSION 3.21)

project(third_party LANGUAGES C CXX)

# spdlog
find_package(spdlog REQUIRED CONFIG)
add_library(spdlog INTERFACE)
target_link_libraries(spdlog INTERFACE spdlog::spdlog)
set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty")

# vma 
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
add_library(vma INTERFACE)
target_link_libraries(vma INTERFACE unofficial::vulkan-memory-allocator::vulkan-memory-allocator)

# volk
find_package(volk CONFIG REQUIRED)
add_library(volk INTERFACE)
target_link_libraries(volk INTERFACE volk::volk volk::volk_headers)