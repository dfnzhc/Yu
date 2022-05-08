cmake_minimum_required(VERSION 3.21)

project(third_party LANGUAGES C CXX)

# spdlog
find_package(spdlog REQUIRED CONFIG)
add_library(spdlog INTERFACE)
target_link_libraries(spdlog INTERFACE spdlog::spdlog)
set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty")