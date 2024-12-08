# Note: Must be included BEFORE project() call
# Solution is experimental in CMake 3.30
cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

# Enable scan for dependencies
set(CXX_SCAN_FOR_MODULES ON)
# disable C++ extensions
set(CMAKE_CXX_EXTENSIONS OFF)
# force C++ version to 23
set(CMAKE_CXX_STANDARD 23)
# standard is required
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the experimental flag
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD
	"0e5b6991-d74f-4b3d-a41c-cf096e0b2508" # current value as per https://www.kitware.com/import-std-in-cmake-3-30/
)

# tell CMake we want to use 'import std' 
# will get enabled for all targets declared after this
set(CMAKE_CXX_MODULE_STD 1)
