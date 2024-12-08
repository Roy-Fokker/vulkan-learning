module;

export module vkl_platform:base_types;

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
}