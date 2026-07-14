#pragma once

#include <filesystem>
#include <string_view>
#include <variant>

namespace io
{
bool write_file(const std::filesystem::path file, std::string_view content);

/**
 * A handle to an anonymous file, that is, a memory-mapped file without a name in the
 * filesystem, making it accessible only through its file descriptor (which is owned
 * by the returned FdStream).
 */
struct AnonymousFile
{
private:
	inline static constexpr int NO_FD = -1;
	int _fd = NO_FD;

	AnonymousFile(int fd) : _fd {fd}
	{
	}
public:
	AnonymousFile() = default;

	static AnonymousFile create()
	{
		std::error_code errc;
		auto file = create(errc);
		if (errc)
		{
			throw std::system_error {errc};
		}

		return file;
	}

	static AnonymousFile create(std::error_code& errc);
	~AnonymousFile();
	AnonymousFile(const AnonymousFile&) = delete;
	AnonymousFile& operator=(const AnonymousFile&) = delete;
	AnonymousFile(AnonymousFile&&) noexcept;
	AnonymousFile& operator=(AnonymousFile&&) noexcept;

	bool is_open() const
	{
		return _fd != NO_FD;
	}

	std::filesystem::path path() const;
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
} // namespace io
