cmake_minimum_required(VERSION 3.21)

project(third_party LANGUAGES C CXX)

# spdlog
set(YUAN_BUILD_TESTS OFF)
add_subdirectory(yuan)
add_library(yuan INTERFACE)
target_link_libraries(yuan INTERFACE Yuan::Yuan)
target_include_directories(yuan INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/yuan/source")
set_target_properties(yuan PROPERTIES FOLDER "ThirdParty")

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
