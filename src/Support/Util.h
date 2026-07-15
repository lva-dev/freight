#pragma once

#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stb/stb_ds.h>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

template<class... Args> void print_error(std::format_string<Args...> fmt, Args... args)
{
	std::println(std::cerr,
		"\033[31merror:\033[39m {}",
		std::format(fmt, std::forward<Args>(args)...));
}

template<class... Args>
[[noreturn]] void bail(std::format_string<Args...> fmt, Args... args)
{
	print_error(fmt, std::forward<Args>(args)...);
	exit(1);
}

template<class... Args> std::string cause(std::format_string<Args...> fmt, Args... args)
{
	return std::format("Caused by:\n  {}", std::format(fmt, std::forward<Args>(args)...));
}

inline std::string cause(std::string_view str)
{
	return std::format("Caused by:\n  {}", str);
}

template<class... Args>
void print_status(const std::string_view status, std::format_string<Args...> fmt, Args... args)
{
	std::print("   \033[32m{}\033[m {}\n",
		status,
		std::format(fmt, std::forward<Args>(args)...));
	std::cout.flush();
}

/**
 * Builder for subprocesses. Contains the path to the executable and the
 * arguments to pass to it.
 */
class ProcessBuilder
{
	std::filesystem::path path_;
	bool nameInferred;
    std::vector<std::string> args;
public:
	ProcessBuilder(const std::filesystem::path& path);

	void set_path(const std::filesystem::path& path);
	void set_name(const std::string& name);
	void add_arg(const std::string& arg);
	void infer_name();

	int start() const;
};

namespace ranges
{
template<class R1, class R2> void move_back_range(R1& dest, R2& src)
{
	dest.insert(dest.end(),
		std::make_move_iterator(src.begin()),
		std::make_move_iterator(src.end()));
}
} // namespace ranges
