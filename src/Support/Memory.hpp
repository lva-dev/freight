#pragma once

#include <boost/optional.hpp>

#include <memory>

namespace supprt::memory
{
template<class T>
auto as_ref(T *ptr) -> boost::optional<T&>
{
	if (ptr != nullptr)
	{
		return *ptr;
	}

	return {};
}

template<class T>
auto as_ref(const T *ptr) -> boost::optional<const T&>
{
	if (ptr != nullptr)
	{
		return *ptr;
	}

	return {};
}

template<class T>
auto as_ref(std::unique_ptr<T>& ptr) -> boost::optional<T&>
{
	return as_ref(ptr.get());
}

template<class T>
auto as_ref(const std::unique_ptr<T>& ptr) -> boost::optional<const T&>
{
	return as_ref(ptr.get());
}

template<class T>
auto as_ref(std::unique_ptr<T>&& ptr) -> boost::optional<const T&> = delete;

template<class T>
auto as_ref(const std::unique_ptr<T>&& ptr) -> boost::optional<const T&> = delete;
} // namespace supprt::memory
