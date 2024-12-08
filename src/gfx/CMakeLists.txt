# find vcpkg installed library
find_package(Vulkan REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(vk-bootstrap CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)

# get platform specific configs
if(MSVC)
	include(${CMAKE_MODULE_PATH}/win32-defs.cmake)
endif()

# get glm specific configs
include(${CMAKE_MODULE_PATH}/glm-defs.cmake)

# set library name variable
set(PRJ_LIB_NAME "vkl-graphics")
set(PRJ_LIB_ALIAS "vkl::graphics")

# create library
add_library(${PRJ_LIB_NAME} SHARED)
add_library(${PRJ_LIB_ALIAS} ALIAS ${PRJ_LIB_NAME})

# set standard to C++ 23
target_compile_features(${PRJ_LIB_NAME} PUBLIC cxx_std_23)

# precompile headers for this library
target_precompile_headers(${PRJ_LIB_NAME}
	PRIVATE
		pch.hpp
)

# source files for this app
target_sources(${PRJ_LIB_NAME}
	PRIVATE
		lib-usage.cpp
	# magic encantations for C++ Modules Support in CMake
	PUBLIC FILE_SET lib_modules TYPE CXX_MODULES FILES
		graphics.ixx
		renderer.ixx
		device.ixx
		command_buffer.ixx
)

list(APPEND vulkan_definitions
	VULKAN_HPP_NO_CONSTRUCTORS            # Vulkan.hpp should not define constructors for structs
	VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1  # Enable dynamic dispatch loader for EXT funcs
	VULKAN_HPP_STORAGE_SHARED             # The storage for the DispatchLoaderDynamic should be embedded in a DLL
	VULKAN_HPP_STORAGE_SHARED_EXPORT      # Export the required symbols.
)

# set preprocessor defines
target_compile_definitions(${PRJ_LIB_NAME}
	PUBLIC
		${platform_definitions}               # Get platform specific definitions
		${vulkan_definitions}                 # Plaform specific Vulkan defines
		${glm_definitions}                    # GLM library configuration
)

# link to libraries used
target_link_libraries(${PRJ_LIB_NAME}
	PRIVATE
		vkl::platform                               # Project's platform lib
		Vulkan::Vulkan                               # Vulkan SDK
		GPUOpen::VulkanMemoryAllocator               # Vulkan Memory Allocator
		vk-bootstrap::vk-bootstrap                   # vk-bootstrap
		vk-bootstrap::vk-bootstrap-compiler-warnings # vk-bootstrap with compiler warning??
		glm::glm                                     # GLM math library
)