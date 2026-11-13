#include "../Pch.hpp"

#include "Process.hpp"

#include <gsl/pointers>
#include <gsl/zstring>

#include <algorithm>
#include <cstring>
#include <expected>
#include <filesystem>
#include <iterator>
#include <string_view>
#include <system_error>

using namespace support::process;

// NOLINTNEXTLINE(readability-identifier-naming)
namespace implementation::unix_
{
static auto execvp(const std::filesystem::path& path,
	std::span<const std::string> args) -> Expected<void>
{
	std::vector<gsl::zstring> exec_args;

	auto zstring_from = [](auto& str) -> auto {
		auto size = str.size();
		auto *ptr = gsl::zstring {new char[size + 1]};
		std::strcpy(ptr, str.data());
		return ptr;
	};

	std::ranges::transform(args, std::back_inserter(exec_args), zstring_from);
	exec_args.emplace_back(nullptr);

	::execvp(path.c_str(), exec_args.data());

	for (auto *zstr : exec_args)
	{
		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
		delete[] zstr;
	}

	return std::unexpected<std::error_code>(std::in_place, errno, std::system_category());
}
} // namespace implementation::unix_

namespace support::process
{
auto ChildHandle::wait() const -> Expected<ChildStatus>
{
	int status = 0;
	int wait_result = waitpid(static_cast<pid_t>(pid_), &status, 0);
    if (wait_result == -1)
    {
        return std::unexpected<std::error_code>(
			std::in_place, errno, std::system_category());
    }

	return ChildStatus {WEXITSTATUS(status)};
}

auto ProcessBuilder::spawn() const -> Expected<ChildHandle>
{
    pid_t pid = ::fork();
	if (pid == -1)
	{
		return std::unexpected<std::error_code>(
			std::in_place, errno, std::system_category());
	}

	if (pid == 0)
	{
		auto exec_result = implementation::unix_::execvp(path_, args_);
		if (!exec_result.has_value())
		{
			return std::unexpected {std::move(exec_result).error()};
		}
	}

	return ChildHandle {pid};
}

auto ProcessBuilder::status() const -> Expected<ChildStatus>
{
    auto spawn_result = spawn();
    if (spawn_result.has_value())
    {
        if (auto wait_result = spawn_result->wait(); wait_result.has_value())
        {
            return wait_result;
        }
        
        return std::unexpected{std::move(spawn_result).error()};
    }

    return std::unexpected{std::move(spawn_result).error()};
}
} // namespace support::process