
# get platform specific configs
if(MSVC)
	include(${CMAKE_MODULE_PATH}/win32-defs.cmake)
endif()

# set library name variable
set(PRJ_LIB_NAME "vkl-platform")
set(PRJ_LIB_ALIAS "vkl::platform")

# create library
add_library(${PRJ_LIB_NAME})
add_library(${PRJ_LIB_ALIAS} ALIAS ${PRJ_LIB_NAME})

# set standard to C++ 23
target_compile_features(${PRJ_LIB_NAME} PUBLIC cxx_std_23)

# precompile headers for this library
target_precompile_headers(${PRJ_LIB_NAME} 
	PUBLIC
		pch.hpp
)

# source files for this app
target_sources(${PRJ_LIB_NAME}
	# magic encantations for C++ Modules Support in CMake
	PUBLIC FILE_SET lib_modules TYPE CXX_MODULES FILES
		platform.ixx
		base_types.ixx
		window.ixx
		input.ixx
		clock.ixx
		console.ixx
		console_hue.ixx
		file_io.ixx
)

# set preprocessor defines
target_compile_definitions(${PRJ_LIB_NAME}
	PUBLIC
		${platform_definitions}     # Get platform specific definitions
)