module;

export module vkl_gfx:renderer;

import std;
import vkl_platform;
import :device;

export namespace vkl::gfx
{
	class renderer final : private vkl::non_copyable
	{
	public:
		renderer() = delete;

		GFX_API renderer(vkl::window::platform_data *data);
		GFX_API ~renderer();

		GFX_API void window_resized();
		GFX_API void update();
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

void renderer::update()
{
}

void renderer::draw()
{
}
