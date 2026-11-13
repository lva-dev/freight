#pragma once

#include <boost/optional.hpp>

#include <optional>

namespace support::optional
{
template<class T>
auto as_ref(std::optional<T>& opt) -> boost::optional<T&>
{
	if (opt.has_value())
	{
		return *opt;
	}

	return {};
}

template<class T>
auto as_ref(const std::optional<T>& opt) -> boost::optional<const T&>
{
	if (opt.has_value())
	{
		return *opt;
	}

	return {};
}

template<class T>
auto as_ref(std::optional<T>&& opt) -> boost::optional<T&> = delete;

template<class T>
auto as_ref(const std::optional<T>&& opt) -> boost::optional<T&> = delete;

template<class T, class F>
constexpr auto transform_std(const boost::optional<T>& opt, F&& f) -> boost::optional<T>&
{
	using Result = std::optional<std::invoke_result_t<F, T>>;

	if (opt.has_value())
	{
		return Result {std::invoke(std::forward<F>(f), *opt)};
	}

	return Result {std::nullopt};
}
} // namespace support::optional
