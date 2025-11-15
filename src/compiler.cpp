#include "compiler.h"

#include <sys/wait.h>
#include <system_error>

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>

namespace {
	class Process {
	private:
		pid_t _pid;

		Process(pid_t pid) : _pid {pid} {};
	public:
		Process(const Process&) = delete;
		Process(Process&&) = delete;
		Process& operator=(const Process&) = default;
		Process& operator=(Process&&) = default;

		/**
		 * @brief Starts a new process.
		 * @param name
		 * @param args
		 * @return a process object that represents the state of the new
		 * process.
		 */
		static Process start(const std::filesystem::path& path,
			std::vector<std::string>& args);

		/**
		 * @brief Waits for this process to end, returning its exit code
		 * after it terminates.
		 * @return the exit code of this process
		 */
		int wait_for();
	};
} // namespace

static std::vector<char *> to_exec_args(const std::vector<std::string>& args) {
	std::vector<char *> c_args;

	for (const auto& str : args) {
		auto c_str = const_cast<char *>(str.data());
		c_args.push_back(c_str);
	}

	c_args.push_back(nullptr);
	return c_args;
}

Process Process::start(const std::filesystem::path& name,
	std::vector<std::string>& args) {
	pid_t pid = fork();

	if (pid == -1) {
		throw std::system_error(errno, std::generic_category());
	}

	if (pid == 0) {
		// we're in the child process
		auto path_native = name.c_str();
		auto exec_args = to_exec_args(args);
		exec_args.insert(exec_args.begin(), const_cast<char *>(name.c_str()));

		if (name.is_absolute()) {
			execv(path_native, exec_args.data());
		} else {
			execvp(path_native, exec_args.data());
		}

		// exec should never return on success, so it has failed here
		throw std::system_error(errno, std::generic_category());
	}

	return Process {pid};
}

int Process::wait_for() {
	int proc_status;
	int wait_result = waitpid(_pid, &proc_status, 0);
	if (wait_result != _pid) {
		// failed to execute child
		throw std::system_error(errno, std::generic_category());
		return false;
	}

	int exitcode = WEXITSTATUS(proc_status);
	return exitcode;
}

std::vector<std::string> freight::GnuCompatibleCompiler::build_args(
	const CompileOpts& opts) {
	if (opts.action.link && opts.action.comp)
		throw std::invalid_argument {""};

	std::vector<std::string> args;

	// Standard
	args.push_back(std::format("-std=c++", opts.std));

	// Warnings
	if (opts.warnings.all)
		args.push_back("-Wall");

	if (opts.warnings.extra)
		args.push_back("-Wextra");

	if (opts.warnings.pedantic)
		args.push_back("-pedantic");

	// Action
	if (opts.action.comp)
		args.push_back("-c");

	// Debug info
	if (opts.debug_info)
		args.push_back("-g");

	// Sanitizer
	if (opts.sanitizer.address)
		args.push_back("-fsanitize=address");

	return args;
}

bool freight::GnuCompatibleCompiler::compile(const CompileOpts& opts,
	std::filesystem::path in_file) {
	// add arguments
	auto args = build_args(opts);

	if (!opts.output_dir.empty()) {
		auto out_file_name = in_file.stem().concat(".o");
		auto out_file_path = opts.output_dir / out_file_name;
		args.push_back("-o");
		args.push_back(out_file_path);
	}

	args.push_back(in_file);

	// execute compiler as new process
	auto process = Process::start(_path, args);
	int exitcode = process.wait_for();
	return exitcode != 0;
}

/**
 * @brief Searches the PATH environment variable for `file`.
 * If the file is found, returns its full path,
 * otherwise returns an empty path.
 * @param file the file to search for
 * @return the full path of the file, or an empty path
 */
std::filesystem::path search_path_variable(std::filesystem::path file) {
	const std::string_view PATH = std::getenv("PATH");

	std::filesystem::path dir;
	for (char ch : PATH) {
		if (ch != ':') {
			dir += ch;
		} else if (!dir.empty()) {
			auto potential_file = dir / file;
			if (std::filesystem::exists(potential_file)) {
				return potential_file;
			}

			dir.clear();
		}
	}

	return {};
}

std::unique_ptr<freight::ClangCompiler> freight::ClangCompiler::find() {
	auto path = search_path_variable("clang++");
	if (path.empty())
		return {};
	else
		return std::unique_ptr<ClangCompiler> {new ClangCompiler {path}};
}

std::unique_ptr<freight::GnuCompiler> freight::GnuCompiler::find() {
	auto path = search_path_variable("g++");
	if (path.empty())
		return {};
	else
		return std::unique_ptr<GnuCompiler> {new GnuCompiler {path}};
}

std::unique_ptr<freight::Compiler> freight::Compiler::from_path(
	const std::filesystem::path& path) {
	if (path == "clang++" || path == "g++")
		return std::unique_ptr<GnuCompatibleCompiler> {
			new GnuCompatibleCompiler {path}};
	else
		return {};
}

std::unique_ptr<freight::Compiler> freight::Compiler::get() {
	auto CXX = getenv("CXX");

	if (CXX != nullptr) {
		auto compiler_path = search_path_variable(CXX);
		if (!compiler_path.empty()) {
			return Compiler::from_path(compiler_path);
		}
	}

	if (auto clang = ClangCompiler::find(); clang != nullptr)
		return clang;
	else if (auto gxx = GnuCompiler::find(); gxx != nullptr)
		return gxx;
	else
		return {};
}