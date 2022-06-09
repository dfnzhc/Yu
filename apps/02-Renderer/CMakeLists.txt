cmake_minimum_required(VERSION 3.21)
project(02-Renderer LANGUAGES CXX C)

add_executable(${PROJECT_NAME} ${PROJECT_NAME}.cpp)
add_dependencies(${PROJECT_NAME} Shaders)

target_link_libraries(${PROJECT_NAME} PRIVATE framework)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
