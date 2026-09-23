#pragma once

#include <concepts>
#include <ranges>

namespace support::ranges
{
namespace concepts
{
	template<class Range, class T>
	concept RangeTo = std::ranges::range<Range> &&
					  std::convertible_to<std::ranges::range_value_t<Range>, T>;

	template<class Range, class T>
	concept ViewOf =
		std::ranges::view<Range> && std::same_as<std::ranges::range_value_t<Range>, T>;
} // namespace concepts

template<class R1, class R2> void move_back_range(R1& dest, R2& src)
{
	dest.insert(dest.end(),
		std::make_move_iterator(src.begin()),
		std::make_move_iterator(src.end()));
}
} // namespace support::ranges
