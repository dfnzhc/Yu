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
 set(IMGUI_FILES
        # header files
        imgui/imconfig.h
        imgui/imgui.h
        imgui/imgui_internal.h
        imgui/imstb_rectpack.h
        imgui/imstb_textedit.h
        imgui/imstb_truetype.h

        # source files
        imgui/imgui.cpp
        imgui/imgui_demo.cpp
        imgui/imgui_draw.cpp
        imgui/imgui_tables.cpp
        imgui/imgui_widgets.cpp
        )

add_library(imgui STATIC ${IMGUI_FILES})
target_include_directories(imgui PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/imgui")
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

