#pragma once

#include <concepts>
#include <generator>
#include <ranges>
#include <type_traits>

#include "Support/Functional.hpp"

namespace support::ranges::concepts
{
template<class Range, class T>
concept RangeTo = std::ranges::range<Range> &&
				  std::convertible_to<std::ranges::range_value_t<Range>, T>;

template<class Range, class T>
concept ViewOf =
	std::ranges::view<Range> && std::same_as<std::ranges::range_value_t<Range>, T>;
} // namespace support::ranges::concepts

namespace support::ranges
{

template<class F, class T = std::invoke_result_t<F>::value_type>
	requires std::is_invocable_r_v<std::optional<std::invoke_result_t<F>>, F>
auto iter_optional(F func) -> std::generator<T>
{
	for (auto val = func(); val.has_value(); val = func())
	{
		co_yield *val;
	}

	co_return;
}

template<class R1, class R2>
void move_back_range(R1& dest, R2& src)
{
	dest.insert(dest.end(),
		std::make_move_iterator(src.begin()),
		std::make_move_iterator(src.end()));
}
} // namespace support::ranges
