#pragma once

#include <expected>
#include <filesystem>
#include <string_view>
#include <variant>

namespace support::io
{
auto write_file(const std::filesystem::path& file, std::string_view content) -> bool;

/**
 * A handle to an anonymous file, that is, a memory-mapped file without a name in the
 * filesystem, making it accessible only through its file descriptor (which is owned
 * by the returned FdStream).
 */
struct AnonymousFile
{
private:
	inline static constexpr const int NO_FD = -1;
	int fd = NO_FD;

	AnonymousFile(int fd) : fd {fd}
	{}

	AnonymousFile() = default;
public:
	static auto create() -> std::expected<AnonymousFile, std::error_code>;

	~AnonymousFile();
	AnonymousFile(const AnonymousFile&) = delete;
	auto operator=(const AnonymousFile&) -> AnonymousFile& = delete;
	AnonymousFile(AnonymousFile&&) noexcept;
	auto operator=(AnonymousFile&&) noexcept -> AnonymousFile&;

	[[nodiscard]] auto is_open() const -> bool
	{
		return fd != NO_FD;
	}

	[[nodiscard]] auto path() const -> std::filesystem::path;
};

/**
 * A handle to an unnamed or named pipe.
 */
// class Pipe
// {
// private:
// 	using Handle = std::variant<std::array<int, 2>, int>;
// 	std::optional<Handle> _id;
// 	bool _named;
// public:
// 	Pipe() = default;
// 	static Pipe create();
// 	static Pipe create_named(const std::filesystem::path& path);
// 	~Pipe();

// 	Pipe& operator=(const Pipe&) = delete;
// 	Pipe(const Pipe&) = delete;
// 	Pipe& operator=(Pipe&&) = default;
// 	Pipe(Pipe&& other) = default;

// 	bool is_open() const
// 	{
// 		return _id.has_value();
// 	}

// 	bool is_unnamed() const
// 	{
// 		return is_open() && !_named;
// 	}

// 	bool is_named() const
// 	{
// 		return is_open() && _named;
// 	}

// 	std::ifstream read_end() const;
// 	std::ofstream write_end() const;
// };
} // namespace support::io
