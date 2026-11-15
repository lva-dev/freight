#pragma once

#include <utility>

namespace support::functional
{
template<class F>
class WithResultOf
{
public:
	using T = decltype(std::declval<F&&>()());

	// NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
	explicit WithResultOf(F&& f) : f(std::forward<F>(f))
	{}

	WithResultOf(const WithResultOf&) = delete;
	auto operator=(const WithResultOf&) -> WithResultOf& = delete;
	WithResultOf(WithResultOf&&) = delete;
	auto operator=(WithResultOf&&) -> WithResultOf& = delete;
	~WithResultOf() = default;

	operator T()
	{
		return f();
	}
private:
	F&& f;
};
} // namespace support::functional

namespace support::functional::concepts
{
template<class F, class R>
concept Supplier = std::is_invocable_r_v<R, F>;

template<class F, class R, class T>
concept Function = std::is_invocable_r_v<R, F, T>;
} // namespace clap::functional::concepts