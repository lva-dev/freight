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
#include <vector>

ProcessBuilder::ProcessBuilder(const std::filesystem::path& path)
	: _path {path},
	  _name_inferred {true}
{
	_args.push_back(path.filename());
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
	_args[0] = name.c_str();
}

void ProcessBuilder::add_arg(const std::string& arg)
{
	_args.push_back(arg);
}

void ProcessBuilder::infer_name()
{
	_name_inferred = true;
	_args[0] = _path.filename().c_str();
}

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
        std::vector<char*> execArgs;
        for (auto& arg : _args) {
            execArgs.push_back(const_cast<char*>(arg.c_str())); // NOLINT
        }
        execArgs.push_back(nullptr);

		execv(_path.c_str(), execArgs.data());
		error_number = errno;
		abort();
	}

	int status = 0;
	waitpid(pid, &status, 0);

	if (error_number != 0)
	{
		int err = error_number;
		bail("Failed to start child process\n\n%s", cause("%s", strerror(err)));
	}

	return WEXITSTATUS(status);
}