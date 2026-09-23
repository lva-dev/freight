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

namespace implementation::unix_
{
static auto execvp(const std::filesystem::path& path,
	std::span<const std::string> args) -> Expected<void>
{
	std::vector<gsl::zstring> execArgs;

	auto zstring_from = [](auto& str) -> auto {
		auto size = str.size();
		auto ptr = gsl::zstring {new char[size + 1]};
		std::strcpy(ptr, str.data());
		return ptr;
	};

	std::ranges::transform(args, std::back_inserter(execArgs), zstring_from);
	execArgs.emplace_back(nullptr);

	::execvp(path.c_str(), execArgs.data());

	for (auto zstr : execArgs)
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
	int waitResult = waitpid(static_cast<pid_t>(pid_), &status, 0);
    if (waitResult == -1)
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
		auto execResult = implementation::unix_::execvp(path_, args_);
		if (!execResult.has_value())
		{
			return std::unexpected {std::move(execResult).error()};
		}
	}

	return ChildHandle {pid};
}

auto ProcessBuilder::status() const -> Expected<ChildStatus>
{
    if (auto spawnResult = spawn(); spawnResult.has_value())
    {
        if (auto waitResult = spawnResult->wait(); waitResult.has_value())
        {
            return waitResult;
        }
        else
        {
            return std::unexpected{std::move(spawnResult).error()};
        }
    }
    else
    {
        return std::unexpected{std::move(spawnResult).error()};
    }
}
} // namespace support::process