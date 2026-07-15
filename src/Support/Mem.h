#pragma once

#include <expected>
#include <system_error>
namespace mem
{
template<class T> struct Shared
{
public:
	Shared() : storage {nullptr} {};

	Shared(const T& value)
	{
		if (auto result = alloc(); !result.has_value())
		{
			throw result.error();
		}

		new (storage) T {value};
	}

	~Shared()
	{
		storage->~T();
		dealloc();
	}

	Shared(const Shared&) = delete;
	Shared& operator=(const Shared&) = delete;
	Shared(Shared&&) = default;
	Shared& operator=(Shared&&) = default;

	bool empty() const
	{
		return storage == nullptr;
	}

	std::expected<void, std::system_error> set(T&& value)
	{
		std::expected<void, std::system_error> result {};
		if (empty())
		{
			result = alloc();
		}
		else
		{
			storage->~T();
		}

		new (storage) T {std::move(value)};
		return result;
	}

	T& get()
	{
		return *storage;
	}

	const T& get() const
	{
		return *storage;
	}

	operator T&()
	{
		return get();
	}

	operator const T&() const
	{
		return get();
	}
private:
	T *storage;

	std::expected<void, std::system_error> alloc();
	void dealloc();
};
}