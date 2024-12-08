module;

export module vkl_gfx:renderer;

import std;
import vkl_platform;
import :device;
import :command_buffer;

export namespace vkl::gfx
{
	class renderer final : private vkl::non_copyable
	{
	public:
		renderer() = delete;

		GFX_API renderer(vkl::window::platform_data *data);
		GFX_API ~renderer();

		GFX_API void window_resized();
		GFX_API void update(const std::array<float, 4> &clear_color);
		GFX_API void draw();

	private:
		std::unique_ptr<vkl::gfx::device> device{ nullptr };
	};
}

using namespace vkl;

renderer::renderer(vkl::window::platform_data *data)
{
	std::println("{}Create renderer.{}",
	             CLR::BLU, CLR::RESET);

	device = std::make_unique<vkl::gfx::device>(data);
}

renderer::~renderer()
{
	std::println("{}Destroy renderer.{}",
	             CLR::BLU, CLR::RESET);
}

void renderer::window_resized()
{
	device->surface_resized();
}

void renderer::update(const std::array<float, 4> &clear_color)
{
	auto current_frame = device->current_image_index();
	current_frame      = device->next_image(current_frame);

	auto extent      = device->get_sc_extent();
	auto clear_value = vk::ClearValue{
		.color = clear_color,
	};
	auto viewport = vk::Viewport{
		.x        = 0.0f,
		.y        = static_cast<float>(extent.height),
		.width    = static_cast<float>(extent.width),
		.height   = static_cast<float>(extent.height) * -1.f,
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	auto scissor = vk::Rect2D{
		.offset = { 0, 0 },
		.extent = extent,
	};

	auto cb = command_buffer{ device->get_cmd_buffer(current_frame) };

	cb.begin();

	cb.end();
}

void renderer::draw()
{
	auto current_frame = device->current_image_index();

	device->submit(current_frame);
	device->present(current_frame);

	device->next_image_index();
}
