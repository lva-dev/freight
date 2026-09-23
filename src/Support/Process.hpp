#pragma once

#include <concepts>
#include <expected>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

#include "Support/Ranges.hpp"

namespace support::process
{
template<class T>
using Expected = std::expected<T, std::error_code>;

class ChildStatus
{
public:
	auto exit_code() const -> int
	{
		return exitCode;
	}
private:
	friend class ChildHandle;

	int exitCode;

	ChildStatus(int exitCode) : exitCode {exitCode}
	{
	}
};

class ChildHandle
{
public:
	auto wait() const -> Expected<ChildStatus>;

	auto pid() const -> std::intmax_t
	{
		return pid_;
	}
private:
	friend class ProcessBuilder;

	std::intmax_t pid_;

	ChildHandle(std::intmax_t pid) : pid_ {pid}
	{
	}
};

/**
 * Builder for subprocesses. Contains the path to the executable and the
 * arguments to pass to it.
 */
class ProcessBuilder
{
	std::filesystem::path path_;
	std::vector<std::string> args_;
public:
	ProcessBuilder(const std::filesystem::path& path) : path_ {path}
	{
		args_.push_back(path.filename());
	}

    auto spawn() const -> Expected<ChildHandle>;
	auto status() const -> Expected<ChildStatus>;

	template<std::convertible_to<std::string> Arg> auto arg(Arg&& arg) -> ProcessBuilder&
	{
		args_.emplace_back(std::forward<Arg>(arg));
		return *this;
	}

	auto args(support::ranges::concepts::RangeTo<std::string> auto&& args) -> ProcessBuilder&
	{
		for (auto&& arg : args)
		{
			args_.emplace_back(std::forward<decltype(arg)>(arg));
		}

		return *this;
	}

	auto get_path() const -> const std::filesystem::path&
	{
		return path_;
	}
};
} // namespace support::process