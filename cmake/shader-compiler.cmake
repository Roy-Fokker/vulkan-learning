# Compile GLSL and HLSL files into SPIR-V files
# Depends on glslc and dxc installed by LunarG SDK

cmake_minimum_required(VERSION 3.29.0 FATAL_ERROR)

# Componenets we need CMake to ensure exist
set (VULKAN_COMPONENTS "dxc" "dxc_exe" "glslc")

# look for above components in Vulkan SDK
find_package(Vulkan REQUIRED COMPONENTS ${VULKAN_COMPONENTS})

# define the shader source function
function(target_shader_sources TARGET)
	cmake_parse_arguments(arg "" "" "HLSL;GLSL" ${ARGN})  # Parse the arguments into GLSL and HLSL groups

	list(LENGTH arg_GLSL glsl_count) # Count the glsl files
	# if count is greater than 0, hand off the glsl_compiler.cmake module 
	if(glsl_count GREATER 0)
		target_glsl_sources(${TARGET} ${arg_GLSL})
	endif()

	list(LENGTH arg_HLSL hlsl_count) # Count the hlsl files
	# if count is greater than 0, hand off the hlsl_compiler.cmake module
	if(hlsl_count GREATER 0)
		target_hlsl_sources(${TARGET} ${arg_HLSL})
	endif()
endfunction()

# usage message
message("The shader_compiler.cmake usage: 
	target_shader_sources(<target> [GLSL <glsl_shader_file> ...] [HLSL <hlsl_shader_file> : <hlsl_shader_profile> ...])
Output:
	${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/<shader_fldr>/<shader_file>.spv ...
")


# Compile glsl files into SPIR-V files
# depends on glslc installed by LunarG SDK
# Usage: target_glsl_sources(<target> [<file> ...])
function (target_glsl_sources TARGET)
	if (NOT TARGET Vulkan::glslc)
		message(FATAL_ERROR "[Error]: Could not find glslc.")
	endif()

	# List of compiled shader output
	set(shader_files "")
	set(shader_sources "")

	foreach(source IN LISTS ARGN)
		# get the absolute path of current source file
		file(REAL_PATH ${source} source_abs)

		if(NOT EXISTS ${source_abs})
			message(FATAL_ERROR "Cannot find shader file: ${source}")
		endif()

		# get only the filename from absolute path
		cmake_path(GET source_abs FILENAME basename)

		# get only the parent directory of the file from absolute path
		cmake_path(GET source_abs PARENT_PATH source_fldr)
		get_filename_component(source_fldr "${source_fldr}" NAME)
		
		# append source shader's parent directory name to output directory path
		set(shader_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source_fldr})

		# full path to compiled output 
		set(output ${shader_dir}/${basename}.spv)

		# TODO: Add shader stage flag for GLSL
		# -fshader-stage=<stage>
		# Treat subsequent input files as having stage <stage>.
		# Valid stages are vertex, vert, fragment, frag, tesscontrol,
		# tesc, tesseval, tese, geometry, geom, compute, and comp.

		# call vulkan sdk's glslc compiler with source and output arguments.
		add_custom_command(
			OUTPUT ${output}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${shader_dir}
			COMMAND Vulkan::glslc ${source_abs} -o ${output} --target-env=vulkan1.3
			DEPENDS ${source_abs}
			COMMENT "GLSLC Compiling SPIRV: ${source} -> ${output}"
			VERBATIM
		)
		list(APPEND shader_sources "${source}")
		list(APPEND shader_files "${output}")
	endforeach()
	
	# make a new variable to hold all output target names
	set(shader_group "${TARGET}_GLSL")
	# add custom target using new variable bound to output file of glslc step
	add_custom_target("${shader_group}"
					  DEPENDS "${shader_files}"
					  SOURCES "${shader_sources}"
	)
	# add compilation of this shader as dependency of the target
	add_dependencies("${TARGET}" "${shader_group}")
endfunction()

# Compile HLSL shaders into SPIR-V files
# depends on dxc installed by LunarG SDK
# Usage: target_shader_sources(<target> [[<file> : <profile>]...])
function(target_hlsl_sources TARGET)
	if (NOT TARGET Vulkan::dxc_exe)
		message(FATAL_ERROR "[Error]: Could not find dxc")
	endif()

	# figure out how many files we have to configure given the pattern
	list(LENGTH ARGN count_HLSL)
	math(EXPR count_HLSL "${count_HLSL} / 3")

	# List of compiled shader output
	set(shader_files "")
	set(shader_sources "")

	# Loop through all the pairs for filename:profile provided
	foreach(i RANGE 1 ${count_HLSL})
		math(EXPR fni "(${i}-1)*3")             # filename index
		math(EXPR pfi "${fni}+2")                # profile index
		list(GET arg_HLSL ${fni} hlsl_filename)
		list(GET arg_HLSL ${pfi} hlsl_profile)

		# get the absolute path of current source file
		file(REAL_PATH ${hlsl_filename} source_abs)

		if(NOT EXISTS ${source_abs})
			message(FATAL_ERROR "Cannot find shader file: ${source_abs}")
		endif()

		# get only the filename from absolute path
		cmake_path(GET source_abs STEM basename)
		set(basename "${basename}.${hlsl_profile}")

		# get only the parent directory of the file from absolute path
		cmake_path(GET source_abs PARENT_PATH source_fldr)
		get_filename_component(source_fldr "${source_fldr}" NAME)
		
		# append source shader's parent directory name to output directory path
		set(shader_dir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${source_fldr})

		# full path to compiled output 
		set(output ${shader_dir}/${basename}.spv)

		# call vulkan sdk's dxc compiler with source and output arguments.
		add_custom_command(
			OUTPUT ${output}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${shader_dir}
			COMMAND Vulkan::dxc_exe -spirv -E main -Fo ${output} -T ${hlsl_profile} ${source_abs}
			DEPENDS ${source_abs}
			COMMENT "DXC Compiling SPIRV: ${hlsl_filename} -> ${output}"
			VERBATIM
		)

		list(APPEND shader_sources "${hlsl_filename}")
		list(APPEND shader_files "${output}")
	endforeach()

	# make a new variable to hold all output target names
	set(shader_group "${TARGET}_HLSL")
	# add custom target using new variable bound to output file of glslc step
	add_custom_target("${shader_group}"
					  DEPENDS "${shader_files}"
					  SOURCES "${shader_sources}"
	)
	# add compilation of this shader as dependency of the target
	add_dependencies("${TARGET}" "${shader_group}")
endfunction()
