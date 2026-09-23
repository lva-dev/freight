#pragma once

#include <boost/optional.hpp>
#include <memory>

namespace supprt::memory
{
	template<class T> boost::optional<T&> as_ref(T *ptr)
	{
		if (ptr != nullptr)
		{
			return *ptr;
		}
		else
		{
			return {};
		}
	}

	template<class T> boost::optional<const T&> as_ref(const T *ptr)
	{
		if (ptr != nullptr)
		{
			return *ptr;
		}
		else
		{
			return {};
		}
	}

	template<class T> boost::optional<T&> as_ref(std::unique_ptr<T>& ptr)
	{
		return as_ref(ptr.get());
	}

	template<class T> boost::optional<const T&> as_ref(const std::unique_ptr<T>& ptr)
	{
		return as_ref(ptr.get());
	}

	template<class T> boost::optional<const T&> as_ref(std::unique_ptr<T>&& ptr) = delete;

	template<class T>
	boost::optional<const T&> as_ref(const std::unique_ptr<T>&& ptr) = delete;
} // namespace memory
