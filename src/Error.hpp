#pragma once

#include <format>
#include <print>
#include <string_view>
#include <utility>

namespace freight::error
{
template<class... Args>
void print_error(std::format_string<Args...> fmt, Args&&...args)
{
	std::println(stderr,
		"\033[31merror:\033[39m {}",
		std::format(fmt, std::forward<Args>(args)...));
}

template<class... Args>
[[noreturn]] void bail(std::format_string<Args...> fmt, Args&&...args)
{
	print_error(fmt, std::forward<Args>(args)...);
	exit(1);
}

template<class... Args>
auto cause(std::format_string<Args...> fmt, Args&&...args) -> std::string
{
	return std::format("Caused by:\n  {}", std::format(fmt, std::forward<Args>(args)...));
}

inline auto cause(std::string_view str) -> std::string
{
	return std::format("Caused by:\n  {}", str);
}

template<class... Args>
void print_status(const std::string_view status,
	std::format_string<Args...> fmt,
	Args&&...args)
{
	std::print("   \033[32m{}\033[m {}\n",
		status,
		std::format(fmt, std::forward<Args>(args)...));
}
} // namespace freight::error