cmake_minimum_required(VERSION 3.21)

project(third_party LANGUAGES C CXX)


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
