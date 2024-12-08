cmake_minimum_required(VERSION 3.30)

# Include Std Modules cmake
# must be included BEFORE project() call
include(${CMAKE_MODULE_PATH}/import-std.cmake)

# project and language to be used
project(Vulkan-Learning-Project
        LANGUAGES CXX)

# library source folder
add_subdirectory(src/platform)

# shader source folder
add_subdirectory(src/shaders)

# executable source folder
add_subdirectory(src/app)