module;

export module vkl_gfx:command_buffer;

import std;

export namespace vkl::gfx
{
	struct command_buffer
	{
		vk::CommandBuffer cb;

		void begin();
		void end();
	};
}

using namespace vkl::gfx;

void command_buffer::begin()
{
	auto begin_info = vk::CommandBufferBeginInfo{};
	auto result     = cb.begin(&begin_info);

	assert(result == vk::Result::eSuccess);
}

void command_buffer::end()
{
	cb.end();
}