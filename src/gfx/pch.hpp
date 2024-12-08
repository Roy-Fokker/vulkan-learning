#pragma once

#ifdef WINDOWS
#include <Windows.h>
#endif

#ifdef WINDOWS
#define GFX_API __declspec(dllexport)
#else
#define GFX_API
#endif

// Include C Assert header
#include <cassert>

// Vulkan C++ wrapper
#include <vulkan/vulkan.hpp>

// Vulkan Bootstrap
#include <VkBootstrap.h>

// Vulkan Memory Allocation
#include <vma/vk_mem_alloc.h>

// GLM math library
#include <glm/glm.hpp>