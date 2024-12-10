module;

export module vkl_platform:file_io;

import std;

export namespace vkl::io
{
	auto read_binary_file(const std::filesystem::path &filename) -> std::vector<std::uint8_t>;
}

// TODO: use ranges to read in data.

using namespace vkl::io;
namespace vw = std::views;
namespace rg = std::ranges;

auto vkl::io::read_binary_file(const std::filesystem::path &filename) -> std::vector<std::uint8_t>
{
	auto file = std::ifstream(filename, std::ios::in | std::ios::binary);

	if (not file.is_open())
	{
		throw std::runtime_error("failed to open file!");
	}

	auto file_size = std::filesystem::file_size(filename);
	auto buffer    = std::vector<uint8_t>(file_size);

	file.read(reinterpret_cast<char *>(buffer.data()), file_size);

	// TODO: code below doesn't work for some reason, sizes don't match.
	// buffer = vw::istream<uint8_t>(file) | rg::to<std::vector>();

	file.close();

	return buffer;
}