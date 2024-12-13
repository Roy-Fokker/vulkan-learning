module;

export module vkl_platform:base_types;

import std;

export namespace vkl
{
	class non_copyable
	{
	protected:
		// Protect constructors no one should be able to create non_copyable base object
		non_copyable()          = default;
		virtual ~non_copyable() = default;

	public:
		// Delete the copy constructor
		non_copyable(const non_copyable &) = delete;

		// Delete the copy assignment operator
		auto operator=(const non_copyable &) -> non_copyable & = delete;

		// Default the move constructor
		non_copyable(non_copyable &&) = default;

		// Default the move assignment operator
		auto operator=(non_copyable &&) -> non_copyable & = default;
	};

	template <class T>
	auto as_byte_span(const std::span<const T> src) -> std::span<const std::byte>
	{
		auto byte_size  = sizeof(T) * src.size();
		auto byte_start = reinterpret_cast<const std::byte *>(src.data());
		return { byte_start, byte_size };
	}

	template <class T>
	auto as_byte_span(const T &src) -> std::span<const std::byte>
	{
		return std::span{
			reinterpret_cast<const std::byte *>(&src),
			sizeof(T)
		};
	}
}