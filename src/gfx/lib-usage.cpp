#pragma once

/*
    VMA requires below to be called EXACTLY once during compile.
    PCH file takes care of it else where.
*/
#ifndef VMA_IMPLEMENTATION
#define VMA_IMPLEMENTATION // Tell Vulkan Memory Allocator that we are using it.
#endif

#include <vma/vk_mem_alloc.h>

/*
    Must be called EXACTLY once for Dynamic Dispatch Loader to work
*/
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE