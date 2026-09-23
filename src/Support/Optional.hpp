#pragma once

#include <boost/optional.hpp>
#include <optional>

namespace support::optional
{
	template<class T> boost::optional<T&> as_ref(std::optional<T>& opt)
	{
		if (opt.has_value())
		{
			return *opt;
		}
		else
		{
			return {};
		}
	}

	template<class T> boost::optional<const T&> as_ref(const std::optional<T>& opt)
	{
		if (opt.has_value())
		{
			return *opt;
		}
		else
		{
			return {};
		}
	}

	template<class T> boost::optional<T&> as_ref(std::optional<T>&& opt) = delete;

	template<class T> boost::optional<T&> as_ref(const std::optional<T>&& opt) = delete;

	template<class T, class F>
	constexpr auto transform_std(const boost::optional<T>& opt, F&& f)
	{
		using Result = std::optional<std::invoke_result_t<F, T>>;

		if (opt)
		{
			return Result {std::invoke(std::forward<F>(f), *opt)};
		}

		return Result {std::nullopt};
	}
} // namespace optional
