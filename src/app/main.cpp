import std;
import vkl_platform;

import application;

namespace CLR = vkl::CLR;

int main()
{
	// if launched from terminal/console, use it to show log messages
	vkl::attach_console();

	// Which directory are we running from?
	std::println("{}Current Path: {} {}",
	             CLR::GRN, std::filesystem::current_path().generic_string(), CLR::RESET);

	auto app = vkl::application();

	return app.run();
}