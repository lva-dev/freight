#include "../Pch.hpp"

#include "Support/Io.hpp"

#include <expected>
#include <utility>

namespace support::io
{
auto write_file(const std::filesystem::path& file, std::string_view content) -> bool
{
	std::ofstream stream {file};
	if (!stream)
	{
		return false;
	}

	stream << content;
	return true;
}

auto AnonymousFile::create() -> std::expected<AnonymousFile, std::error_code>
{
	static constexpr int NO_FLAGS = 0;
	int fd = memfd_create("", NO_FLAGS);
	if (fd == -1)
	{
		return std::unexpected<std::error_code>(
			std::in_place, errno, std::system_category());
	}
	else
	{
		return AnonymousFile {fd};
	}

	return AnonymousFile {fd};
}

AnonymousFile::~AnonymousFile()
{
	if (fd != NO_FD)
	{
		::close(fd);
	}
}

AnonymousFile::AnonymousFile(AnonymousFile&& other) noexcept
	: fd {std::exchange(other.fd, NO_FD)}
{}

auto AnonymousFile::operator=(AnonymousFile&& other) noexcept -> AnonymousFile&
{
	fd = std::exchange(other.fd, NO_FD);
	return *this;
}

auto AnonymousFile::path() const -> std::filesystem::path
{
	return std::format("/proc/self/fd/{}", fd);
}

// Pipe Pipe::create()
// {
// 	// TODO: pipe()
// 	return {};
// }

// Pipe Pipe::create_named(const std::filesystem::path& path)
// {
// 	// TODO: mkfifo()
// 	return {};
// }

// Pipe::~Pipe()
// {
// 	if (is_named())
// 	{
// 		close(std::get<1>(*_id));
// 	}
// 	else if (is_unnamed())
// 	{
// 		auto& fds = std::get<0>(*_id);
// 		close(fds[0]);
// 		close(fds[1]);
// 	}
// }

// std::ifstream Pipe::read_end() const
// {
// 	int fd = (is_named()) ? std::get<1>(*_id) : std::get<0>(*_id)[0];
// 	return std::ifstream {std::format("/proc/self/fd/{}", fd)};
// }

// std::ofstream Pipe::write_end() const
// {
// 	int fd = (is_named()) ? std::get<1>(*_id) : std::get<0>(*_id)[1];
// 	return std::ofstream {std::format("/proc/self/fd/{}", fd)};
// }
} // namespace support::io
