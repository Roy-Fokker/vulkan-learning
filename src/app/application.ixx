module;

export module application;

import std;
import vkl_platform;
import vkl_gfx;

export namespace vkl
{
	class application final : private non_copyable
	{
	public:
		application();
		~application() = default;

		auto run() -> uint32_t;

	private:
		auto on_close(const vkl::window::close &data) -> bool;
		auto on_resize(const vkl::window::resize &data) -> bool;
		auto on_activate(const vkl::window::activate &data) -> bool;

		void setup_renderer();
		void update_on_input();

	private:
		bool stop_app      = false;
		bool capture_mouse = false;

		vkl::clock::timer clock;
		std::unique_ptr<vkl::window> window{ nullptr };
		std::unique_ptr<vkl::input> input{ nullptr };

		std::unique_ptr<vkl::gfx::renderer> renderer{ nullptr };
	};

}

using namespace vkl;
using namespace std::string_view_literals;

namespace vkl_clock = vkl::clock;

application::application()
{
	// High resolution clock via std::chrono
	clock = vkl::clock::timer();

	// Platform window wrapper
	window = std::make_unique<vkl::window>(
		window::description{
			.width  = 1280,
			.height = 768,
			.title  = L"Vulkan Learnings"sv,
			.style  = vkl::window::style::sizeable,
		});

	// Window event callbacks
	using namespace std::placeholders;
	window->on_event(std::bind(&application::on_close, this, _1));
	window->on_event(std::bind(&application::on_resize, this, _1));
	window->on_event(std::bind(&application::on_activate, this, _1));

	// Input handler using RawInput on Windows
	input = std::make_unique<vkl::input>(
		window->get_platform_data(),
		std::array{ vkl::input_device::keyboard, vkl::input_device::mouse });

	// Vulkan Renderer
	renderer = std::make_unique<vkl::gfx::renderer>(window->get_platform_data());
}

auto application::on_close([[maybe_unused]] const vkl::window::close &data) -> bool
{
	std::println("{}Window Closed.{}", CLR::YEL, CLR::RESET);
	stop_app = true;
	return true;
}

auto application::on_resize([[maybe_unused]] const vkl::window::resize &data) -> bool
{
	std::println("{}Window Resized:{} {}x{}", CLR::YEL, CLR::RESET, data.width, data.height);
	if (renderer)
	{
		renderer->window_resized();
		return true;
	}

	input->toggle_mouse_lock(capture_mouse); // Resize capture zone

	return false;
}

auto application::on_activate([[maybe_unused]] const vkl::window::activate &data) -> bool
{
	auto state = data.current_state == vkl::window::state::active ? true : false;
	std::println("{}Window Activate: {}{}", CLR::YEL, CLR::RESET, state);

	input->toggle_mouse_lock(capture_mouse and state); // only capture if active window

	return false;
}

auto application::run() -> uint32_t
{
	setup_renderer();

	window->toggle_show();
	clock.reset();

	while (not stop_app)
	{
		window->pump_messages();
		input->process_messages();

		renderer->update(std::array{ 0.4f, 0.4f, 0.7f, 0.0f });
		renderer->draw();

		clock.tick();

		update_on_input();
	}

	return 0;
}

void application::setup_renderer()
{
	auto vs_bin = vkl::io::read_binary_file("shaders/basic_shader.vs_6_4.spv");
	auto fs_bin = vkl::io::read_binary_file("shaders/basic_shader.ps_6_4.spv");

	renderer->add_pipeline(vs_bin, fs_bin);
}

void application::update_on_input()
{
	using btn  = vkl::input_button;
	using axis = vkl::input_axis;

	auto dt = clock.get_delta<vkl_clock::s>(); // get time since previous call to this function/reset
	auto tt = clock.get_total<vkl_clock::s>(); // get total time since call to reset

	// if escape is pressed, stop app
	if (input->is_button_down(btn::escape))
	{
		stop_app = true;
		std::println("⌚ : {:>5.2f}s, ⏱️ : {:.6f}s, 🔽 : {:10.10}",
		             tt, dt, vkl::to_string(btn::escape));
	}
}