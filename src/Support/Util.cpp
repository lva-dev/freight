#include "../Pch.h"

#include "Support/Util.h"
#include "Support/Mem.h"

ProcessBuilder::ProcessBuilder(const std::filesystem::path& path)
	: path_ {path},
	  nameInferred {true}
{
	args.push_back(path.filename());
}

void ProcessBuilder::set_path(const std::filesystem::path& path)
{
	path_ = path;

	if (nameInferred)
	{
		infer_name();
	}
}

void ProcessBuilder::set_name(const std::string& name)
{
	nameInferred = false;
	args[0] = name.c_str();
}

void ProcessBuilder::add_arg(const std::string& arg)
{
	args.push_back(arg);
}

void ProcessBuilder::infer_name()
{
	nameInferred = true;
	args[0] = path_.filename().c_str();
}

namespace mem {

template<class T> std::expected<void, std::system_error> Shared<T>::alloc()
{
	assert(storage != nullptr &&
		   "Tried to reallocate non-null storage. Call `Shared::dealloc` first");
	static constexpr int PROT_FLAGS = PROT_READ | PROT_WRITE;
	static constexpr int MAP_FLAGS = MAP_SHARED | MAP_ANONYMOUS;
	storage = static_cast<T *>(mmap(nullptr, sizeof(T), PROT_FLAGS, MAP_FLAGS, -1, 0));
	if (storage == nullptr)
	{
		return std::unexpected(std::system_error {errno, std::system_category()});
	}
	else
	{
		return {};
	}
}

template<class T> void Shared<T>::dealloc()
{
	assert(storage == nullptr && "Tried to deallocate null. Call `Shared::alloc` first");
	int result = munmap(storage, sizeof(T));
	assert(result != -1 && "munmap should never fail");
	storage = nullptr;
}

} // namespace mem

int ProcessBuilder::start() const
{
	using namespace std::filesystem;

	assert(path_.is_absolute() && exists(path_));

	mem::Shared<int> errorNumber;
	try
	{
		errorNumber = 0;
	}
	catch (std::system_error& e)
	{
		bail("Failed to start child process\n\n%s",
			cause("%s", strerror(e.code().value())));
	}

	pid_t pid = fork();

	if (pid == -1)
	{
		errorNumber = errno;
	}

	if (pid == 0)
	{
		std::vector<char *> execArgs;
		for (auto& arg : args)
		{
			execArgs.push_back(const_cast<char *>(arg.c_str())); // NOLINT
		}
		execArgs.push_back(nullptr);

		execv(path_.c_str(), execArgs.data());
		errorNumber = errno;
		abort();
	}

	int status = 0;
	waitpid(pid, &status, 0);

	if (errorNumber != 0)
	{
		int err = errorNumber;
		bail("Failed to start child process\n\n%s", cause("%s", strerror(err)));
	}

	return WEXITSTATUS(status);
}