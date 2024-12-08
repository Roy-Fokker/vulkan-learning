# find vcpkg installed library
find_package(glm CONFIG REQUIRED)

# get platform specific configs
if(MSVC)
	include(${CMAKE_MODULE_PATH}/win32-defs.cmake)
endif()

include(${CMAKE_MODULE_PATH}/glm-defs.cmake)

# name of this application/library
set(PRJ_APP_NAME "VL-App")

# create executable for initial setup
add_executable(${PRJ_APP_NAME})

# ensure it's Standard C++ 23
target_compile_features(${PRJ_APP_NAME} PRIVATE cxx_std_23)

# set preprocessor defines
target_compile_definitions(${PRJ_APP_NAME}
	PRIVATE
		${platform_definitions}     # Get platform specific definitions
		${glm_definitions}          # GLM Configuration
)

# set linker options
target_link_options(${PRJ_APP_NAME}
	PRIVATE
		${executable_link_options}
)

# precompile headers for this library
target_precompile_headers(${PRJ_APP_NAME}
	PRIVATE
		pch.hpp
)

# source files for this application
target_sources(${PRJ_APP_NAME}
	# Non C++ module source files
	PRIVATE
		main.cpp

	# magic encantations for C++ Modules Support in CMake
	PRIVATE FILE_SET app_modules TYPE CXX_MODULES FILES
		application.ixx
)

# libraries used by this application
target_link_libraries(${PRJ_APP_NAME}
	PRIVATE
		vkl::platform
		vkl::graphics
		vkl::shaders
		glm::glm
)