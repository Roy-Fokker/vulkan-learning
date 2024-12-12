module;

export module vkl_gfx:buffer;

import std;

export namespace vkl::gfx
{
	class buffer final
	{
	public:
		struct description
		{
			uint32_t allocation_size;
			vk::BufferUsageFlags usage;
			VmaMemoryUsage memory_usage;
		};

	public:
		buffer(VmaAllocator allocator, const description &desc);
		~buffer();

		void copy(std::span<const std::byte> data);

		auto get_buffer() const -> vk::Buffer;
		auto get_gpu_address(vk::Device device) const -> vk::DeviceAddress;

	private:
		VmaAllocator allocator;

		vk::Buffer gpu_buffer;
		VmaAllocation allocation;
		VmaAllocationInfo info;
	};
}

using namespace vkl::gfx;

buffer::buffer(VmaAllocator allocator, const description &desc)
{
	auto gpu_buffer_info = vk::BufferCreateInfo{
		.size  = desc.allocation_size,
		.usage = desc.usage,
	};

	auto vma_alloc_info = VmaAllocationCreateInfo{
		.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT,
		.usage = desc.memory_usage,
	};

	auto result = (vk::Result)vmaCreateBuffer(
		allocator,
		reinterpret_cast<VkBufferCreateInfo *>(&gpu_buffer_info),
		&vma_alloc_info,
		reinterpret_cast<VkBuffer *>(&gpu_buffer),
		&allocation,
		&info);
	assert(result == vk::Result::eSuccess);
}

buffer::~buffer()
{
	vmaDestroyBuffer(allocator, gpu_buffer, allocation);
}

void buffer::copy(std::span<const std::byte> src_data)
{
	void *dst_data = nullptr;

	vmaMapMemory(allocator, allocation, &dst_data);
	memcpy(dst_data, src_data.data(), src_data.size());
	vmaUnmapMemory(allocator, allocation);
}

auto buffer::get_buffer() const -> vk::Buffer
{
	return gpu_buffer;
}

auto buffer::get_gpu_address(vk::Device device) const -> vk::DeviceAddress
{
	auto buf_addr_info = vk::BufferDeviceAddressInfo{
		.buffer = gpu_buffer,
	};
	return device.getBufferAddress(&buf_addr_info);
}
