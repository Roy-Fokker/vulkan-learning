#include logic to build shader files/targets
include(${CMAKE_MODULE_PATH}/shader-compiler.cmake)

# set shader library name variable
set(PRJ_SHADER_LIB "shaders")
set(PRJ_SHADER_LIB_ALIAS "vkl::shaders")

# create library
add_library(${PRJ_SHADER_LIB} INTERFACE)
add_library(${PRJ_SHADER_LIB_ALIAS} ALIAS ${PRJ_SHADER_LIB})

# shader sources
target_shader_sources(${PRJ_SHADER_LIB}
	HLSL
		basic_shader.vs.hlsl : vs_6_4
		basic_shader.ps.hlsl : ps_6_4
		basic_mesh.vs.hlsl : vs_6_4
		basic_color_blend.vs.hlsl : vs_6_4
)