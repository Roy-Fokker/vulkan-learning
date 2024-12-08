module;

export module vkl_platform:window;

import std;
import :base_types;
import :console_hue;

export namespace vkl
{
	class window : private non_copyable
	{
	public:
		struct platform_data; // defined per platform

		enum class style : std::uint8_t
		{
			sizeable,
			fixed,
			borderless,
			fullscreen,
		};

		enum class state : std::uint8_t
		{
			active,
			inactive,
		};

		struct description
		{
			std::uint16_t width     = 800;
			std::uint16_t height    = 600;
			std::wstring_view title = L"Window Name";
			style style             = style::sizeable;
			// TODO: Window Icon???
		};

		struct resize
		{
			std::uint16_t width;
			std::uint16_t height;
		};
		struct activate
		{
			state current_state;
			bool minimized;
		};
		struct close
		{
		};

		using resize_callback   = std::function<bool(const resize &data)>;
		using activate_callback = std::function<bool(const activate &data)>;
		using close_callback    = std::function<bool(const close &data)>;

		using event = std::variant<resize, activate, close>;

	public:
		window() = default;

		window(const description &desc);
		~window();

		void change_style(style new_style);
		void change_size(std::uint16_t width, std::uint16_t height);
		void toggle_show();

		void pump_messages();

		void on_event(const resize_callback &cb_resize);
		void on_event(const activate_callback &cb_activate);
		void on_event(const close_callback &cb_close);

		[[nodiscard]]
		auto get_platform_data() const -> platform_data *;

		[[nodiscard]]
		auto get_window_size() const -> std::array<std::uint16_t, 2>;

		auto call_event(const event &evt) -> int;

	private:
		std::unique_ptr<platform_data> internal_data = nullptr;

		resize_callback on_resize     = nullptr;
		activate_callback on_activate = nullptr;
		close_callback on_close       = nullptr;
	};
}

// implementation per platform
using namespace vkl;
using namespace std::string_view_literals;

#ifdef WINDOWS
namespace
{
	// helper type for the visitor
	// https://en.cppreference.com/w/cpp/utility/variant/visit2
	template <class... Ts>
	struct overloads : Ts...
	{
		using Ts::operator()...;
	};

	constexpr auto CLASSNAME = L"Win32Window"sv;

	auto window_style(window::style style) -> std::array<DWORD, 2>
	{
		switch (style)
		{
			using enum window::style;
			case fixed:
				return {
					WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
					WS_EX_OVERLAPPEDWINDOW
				};
				break;
			case sizeable:
				return {
					WS_OVERLAPPEDWINDOW,
					WS_EX_OVERLAPPEDWINDOW
				};

				break;
			case borderless:
			case fullscreen:
				return {
					WS_POPUP | WS_MINIMIZEBOX,
					NULL
				};
				break;
		}

		return {};
	}

	auto modify_style(DWORD base, DWORD remove, DWORD add) -> DWORD
	{
		return (base & ~(remove)) | add;
	}

	auto centre_on_screen(RECT rect) -> std::array<std::uint16_t, 4>
	{
		auto [x, y, w, h] = rect; // split out adjusted rectangle

		// centre window on screen
		w = w - x;
		h = h - y;
		x = (GetSystemMetrics(SM_CXSCREEN) - w) / 2;
		y = (GetSystemMetrics(SM_CYSCREEN) - h) / 2;

		SetCursorPos(x + w / 2, y + h / 2); // centre mouse in the window

		return {
			(uint16_t)x,
			(uint16_t)y,
			(uint16_t)w,
			(uint16_t)h
		};
	}

	auto adjust_window_size(std::uint16_t width, std::uint16_t height, DWORD style, DWORD style_ex) -> RECT
	{
		auto wnd_rect = RECT{ 0, 0, width, height };          // desired window size
		AdjustWindowRectEx(&wnd_rect, style, NULL, style_ex); // adjust window size, ensure client area matches desc
		return wnd_rect;
	}

	LRESULT CALLBACK window_procedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (msg == WM_NCCREATE)
		{
			auto windowPtr = reinterpret_cast<std::int64_t>(reinterpret_cast<LPCREATESTRUCTW>(lParam)->lpCreateParams);
			SetWindowLongPtr(hWnd,
			                 GWLP_USERDATA,
			                 windowPtr);
		}

		auto wnd = reinterpret_cast<window *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		if (not wnd)
		{
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}

		auto handled = [&](int callback_value) -> LRESULT {
			if (callback_value == -1)
			{
				return DefWindowProc(hWnd, msg, wParam, lParam);
			}
			return callback_value;
		};

		switch (msg)
		{
			case WM_CLOSE:
			{
				return handled(wnd->call_event(window::close{}));
			}
			case WM_WINDOWPOSCHANGED:
			{
				auto win_pos = reinterpret_cast<WINDOWPOS *>(lParam);

				auto width  = static_cast<std::uint16_t>(win_pos->cx);
				auto height = static_cast<std::uint16_t>(win_pos->cy);

				return handled(
					wnd->call_event(window::resize{
						.width  = width,
						.height = height }));
			}
			case WM_ACTIVATE:
			{
				auto is_active    = (LOWORD(wParam) | WA_INACTIVE)
				                      ? window::state::active
				                      : window::state::inactive;
				auto is_minimized = bool(HIWORD(wParam));

				return handled(
					wnd->call_event(window::activate{
						.current_state = is_active,
						.minimized     = is_minimized }));
			}
		}

		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
}

struct window::platform_data
{
	HINSTANCE hInstance;
	HWND hWnd;
};

window::window(const description &desc)
{
	internal_data            = std::make_unique<platform_data>();
	internal_data->hInstance = GetModuleHandle(nullptr);

	BOOL dpi_result = SetProcessDPIAware();
	assert(dpi_result == TRUE);

	// Register Window Class
	auto wc = WNDCLASSEXW{
		.cbSize        = sizeof(WNDCLASSEX),
		.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
		.lpfnWndProc   = window_procedure,
		.hInstance     = internal_data->hInstance,
		.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH)),
		.lpszClassName = CLASSNAME.data()
	};
	RegisterClassEx(&wc);

	// figure out window size and location
	auto [wnd_style, wnd_style_ex] = window_style(desc.style); // convert enum to win32 values
	auto wnd_rect                  = adjust_window_size(desc.width, desc.height, wnd_style, wnd_style_ex);
	auto [x, y, w, h]              = centre_on_screen(wnd_rect);

	// create window
	internal_data->hWnd = CreateWindowEx(wnd_style_ex,
	                                     CLASSNAME.data(),
	                                     desc.title.data(),
	                                     wnd_style,
	                                     x, y, w, h,
	                                     nullptr, nullptr,
	                                     internal_data->hInstance,
	                                     static_cast<LPVOID>(this) // so Window Proc can pull this object out
	);

	std::println("{}Create window.{}", CLR::BLU, CLR::RESET);
}

window::~window()
{
	std::println("{}Destroy window.{}", CLR::BLU, CLR::RESET);

	// Clear out the call backs
	on_resize   = nullptr;
	on_activate = nullptr;
	on_close    = nullptr;

	// Post this message to stop windows messages from coming through
	PostQuitMessage(NULL);

	// Destroy Window
	if (internal_data->hWnd)
	{
		DestroyWindow(internal_data->hWnd);
		internal_data->hWnd = nullptr;
	}

	// Unregister Window Class
	HINSTANCE hInstance = GetModuleHandle(nullptr);
	UnregisterClass(CLASSNAME.data(), hInstance);
}

void window::change_style(style new_style)
{
	constexpr DWORD CLEAR_STYLE    = WS_POPUP | WS_BORDER | WS_MINIMIZEBOX | WS_OVERLAPPEDWINDOW;
	constexpr DWORD CLEAR_STYLE_EX = WS_EX_OVERLAPPEDWINDOW | WS_EX_LAYERED | WS_EX_COMPOSITED;

	auto hWnd = internal_data->hWnd;

	auto [new_wnd_style, new_wnd_style_ex] = window_style(new_style);

	auto wnd_style    = static_cast<DWORD>(::GetWindowLongPtrW(hWnd, GWL_STYLE));
	auto wnd_style_ex = static_cast<DWORD>(::GetWindowLongPtr(hWnd, GWL_EXSTYLE));

	auto wnd_rect = RECT{};
	GetClientRect(hWnd, &wnd_rect);
	AdjustWindowRectEx(&wnd_rect, new_wnd_style, NULL, new_wnd_style_ex);

	wnd_style    = modify_style(wnd_style, CLEAR_STYLE, new_wnd_style);
	wnd_style_ex = modify_style(wnd_style_ex, CLEAR_STYLE_EX, new_wnd_style_ex);

	SetWindowLong(hWnd, GWL_STYLE, wnd_style);
	SetWindowLong(hWnd, GWL_EXSTYLE, wnd_style_ex);

	// centre window on screen
	auto [x, y, w, h] = centre_on_screen(wnd_rect);
	MoveWindow(hWnd, x, y, w, h, FALSE);
}

void window::change_size(std::uint16_t width, std::uint16_t height)
{
	auto hWnd         = internal_data->hWnd;
	auto wnd_style    = static_cast<DWORD>(::GetWindowLongPtrW(hWnd, GWL_STYLE));
	auto wnd_style_ex = static_cast<DWORD>(::GetWindowLongPtr(hWnd, GWL_EXSTYLE));
	auto wnd_rect     = adjust_window_size(width, height, wnd_style, wnd_style_ex);
	auto [x, y, w, h] = centre_on_screen(wnd_rect);
	MoveWindow(hWnd, x, y, w, h, FALSE);
}

void window::toggle_show()
{
	static auto is_visible = false;
	auto hWnd              = internal_data->hWnd;

	if (not is_visible)
	{
		ShowWindow(hWnd, SW_SHOWNORMAL);
		SetFocus(hWnd);
	}
	else
	{
		ShowWindow(hWnd, SW_HIDE);
	}
	is_visible = not is_visible;
}

void window::pump_messages()
{
	constexpr auto MSG_MAX{ 0xFFFFFFFF };

	auto process_range = [](UINT min, UINT max) {
		auto has_more_messages = BOOL{ TRUE };
		while (has_more_messages)
		{
			MSG msg{};

			// TODO: verify below statement is true. it seems to work with Handle provided.
			// Parameter two here has to be nullptr, putting hWnd here will
			// not retrive WM_QUIT messages, as those are posted to the thread
			// and not the window
			has_more_messages = PeekMessage(&msg, nullptr, min, max, PM_NOYIELD | PM_REMOVE);
			if (msg.message == WM_QUIT)
			{
				return;
			}

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	};

	// Avoid process WM_INPUT, for raw input to work
	process_range(0, (WM_INPUT - 1));
	process_range((WM_INPUT + 1), MSG_MAX);
}

void window::on_event(const resize_callback &cb_resize)
{
	on_resize = cb_resize;
}

void window::on_event(const activate_callback &cb_activate)
{
	on_activate = cb_activate;
}

void window::on_event(const close_callback &cb_close)
{
	on_close = cb_close;
}

[[nodiscard]]
auto window::get_platform_data() const -> platform_data *
{
	return internal_data.get();
}

[[nodiscard]]
auto window::get_window_size() const -> std::array<std::uint16_t, 2>
{
	RECT rect{};
	GetClientRect(internal_data->hWnd, &rect);

	return {
		static_cast<std::uint16_t>(rect.right - rect.left),
		static_cast<std::uint16_t>(rect.bottom - rect.top)
	};
}

auto window::call_event(const event &evt) -> int
{
	auto resize_event = [&](const resize &data) -> int {
		if (on_resize)
		{
			// auto data = std::get<resize>(evt);
			return on_resize(data);
		}
		return -1;
	};
	auto activate_event = [&](const activate &data) -> int {
		if (on_activate)
		{
			// auto data = std::get<activate>(evt);
			return on_activate(data);
		}
		return -1;
	};
	auto close_event = [&](const close &data) -> int {
		if (on_close)
		{
			// auto data = std::get<close>(evt);
			return on_close(data);
		}
		return -1;
	};

	return std::visit(overloads{ resize_event, activate_event, close_event }, evt);
}

#endif

#ifdef LINUX
namespace
{
	// TODO: implement linux side
}
#endif