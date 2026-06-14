#include "Pch.h"

#include "Util.h"

#include <cstdlib>
#include <format>
#include <fstream>
#include <ios>
#include <string>
#include <sys/stat.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace io
{
bool write_file(const std::filesystem::path file, std::string_view content)
{
	std::ofstream stream {file};
	if (!stream)
	{
		return false;
	}

	stream << content;
	return true;
}

AnonymousFile AnonymousFile::create(std::error_code& errc)
{
	static constexpr int NO_FLAGS = 0;
	int fd = memfd_create("", NO_FLAGS);
	if (fd == -1)
	{
		errc = std::error_code(errno, std::system_category());
	}
	else
	{
		return AnonymousFile {fd};
	}

	return AnonymousFile {fd};
}

AnonymousFile::~AnonymousFile()
{
	if (_fd != NO_FD)
	{
		close(_fd);
	}
}

AnonymousFile::AnonymousFile(AnonymousFile&& other)
	: _fd {std::exchange(other._fd, NO_FD)}
{
}

AnonymousFile& AnonymousFile::operator=(AnonymousFile&& other)
{
	_fd = std::exchange(other._fd, NO_FD);
	return *this;
}

std::filesystem::path AnonymousFile::path() const
{
	return std::format("/proc/self/fd/{}", _fd);
}

Pipe Pipe::create()
{
	// TODO: pipe()
	return {};
}

Pipe Pipe::create_named(const std::filesystem::path& path)
{
	// TODO: mkfifo()
	return {};
}

Pipe::~Pipe()
{
	if (is_named())
	{
		close(std::get<1>(*_id));
	}
	else if (is_unnamed())
	{
		auto& fds = std::get<0>(*_id);
		close(fds[0]);
		close(fds[1]);
	}
}

std::ifstream Pipe::read_end() const
{
	int fd = (is_named()) ? std::get<1>(*_id) : std::get<0>(*_id)[0];
	return std::ifstream {std::format("/proc/self/fd/{}", fd)};
}

std::ofstream Pipe::write_end() const
{
	int fd = (is_named()) ? std::get<1>(*_id) : std::get<0>(*_id)[1];
	return std::ofstream {std::format("/proc/self/fd/{}", fd)};
}
} // namespace io

ProcessBuilder::ProcessBuilder(const std::filesystem::path& path)
	: _path {path},
	  _name_inferred {true}
{
	_args.push_back(path.filename().c_str());
}

void ProcessBuilder::set_path(const std::filesystem::path& path)
{
	_path = path;

	if (_name_inferred)
	{
		infer_name();
	}
}

void ProcessBuilder::set_name(const std::string& name)
{
	_name_inferred = false;
	_args.set(0, name.c_str());
}

void ProcessBuilder::add_arg(const std::string& arg)
{
	_args.push_back(arg);
}

void ProcessBuilder::infer_name()
{
	_name_inferred = true;
	_args.set(0, _path.filename().c_str());
}

/**
 * Clones `str` as a dynamically-allocated c-string.
 * Returned string should be freed with `delete[]`.
 */
static char *clone_as_c_str(std::string_view str)
{
	char *c_str = new char[str.length() + 1];
	std::memcpy(c_str, str.data(), str.length());
	c_str[str.length()] = '\0';
	return c_str;
}

namespace ds
{
void Strings::set(std::size_t i, std::string_view str)
{
	delete[] _data[i];
	_data[i] = strndup(str.data(), str.size());
	return;
}

void Strings::insert(std::size_t i, std::string_view str)
{
	assert(i <= size());
	_data.insert(_data.begin() + i, clone_as_c_str(str));
}

void Strings::erase(std::size_t i)
{
	assert(i < size());
	auto str = _data[i];
	delete[] str;
	_data.erase(_data.begin() + i);
}
} // namespace ds

namespace mem
{
template<class T> struct Shared
{
private:
	T *_storage;
	void alloc();
	void dealloc();
public:
	Shared();
	Shared(const T& value);
	~Shared();
	Shared(const Shared&) = delete;
	Shared& operator=(const Shared&) = delete;
	Shared(Shared&&) = default;
	Shared& operator=(Shared&&) = default;

	bool empty() const
	{
		return _storage == nullptr;
	}

	Shared& operator=(const T& value);
	operator T&();
};

template<class T> void Shared<T>::alloc()
{
	constexpr int PROT_FLAGS = PROT_READ | PROT_WRITE;
	constexpr int MAP_FLAGS = MAP_SHARED | MAP_ANONYMOUS;
	_storage = static_cast<T *>(mmap(nullptr, sizeof(T), PROT_FLAGS, MAP_FLAGS, -1, 0));
	if (_storage == nullptr)
	{
		throw std::system_error {errno, std::system_category()};
	}
}

template<class T> void Shared<T>::dealloc()
{
	int result = munmap(_storage, sizeof(T));
	assert(result != -1 && "munmap should never fail");
	_storage = nullptr;
}

template<class T> Shared<T>::Shared() : _storage {nullptr}
{
}

template<class T> Shared<T>::Shared(const T& value) : Shared {}
{
	alloc();
	new (_storage) T {value};
}

template<class T> Shared<T>::~Shared()
{
	_storage->~T();
	dealloc();
}

template<class T> Shared<T>& Shared<T>::operator=(const T& value)
{
	if (empty())
	{
		alloc();
	}
	else
	{
		_storage->~T();
	}

	new (_storage) T {value};
	return *this;
}

template<class T> Shared<T>::operator T&()
{
	return *_storage;
}
} // namespace mem

int ProcessBuilder::start() const
{
	using namespace std::filesystem;

	assert(_path.is_absolute() && exists(_path));

	mem::Shared<int> error_number;
	try
	{
		error_number = 0;
	}
	catch (std::system_error& e)
	{
		bail("Failed to start child process\n\n%s",
			cause("%s", strerror(e.code().value())));
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		error_number = errno;
	}

	if (pid == 0)
	{
		execv(_path.c_str(), _args.data());
		error_number = errno;
		abort();
	}

	int status;
	waitpid(pid, &status, 0);

	if (error_number != 0)
	{
		int err = error_number;
		bail("Failed to start child process\n\n%s", cause("%s", strerror(err)));
	}

	return WEXITSTATUS(status);
}