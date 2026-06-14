#include "pch.h"

#include "compiler.h"

#include <filesystem>
#include <memory>
#include <ranges>
#include <system_error>

namespace freight {
//////////////////////////////////////////////////
/// Process
//////////////////////////////////////////////////

Process::~Process() {
	if (pid_ != NO_PID) {
		std::terminate();
	}
}

static char *string_pointer(const std::string& str) {
	return const_cast<char *>(str.data());
}

static std::vector<char *> to_args_for_exec(
	const std::span<std::string>& args) {

	using namespace std::ranges;
	auto c_args =
		to<std::vector<char *>>(views::transform(args, string_pointer));
	c_args.push_back(nullptr);
	return c_args;
}

Process Process::start(const std::filesystem::path& name,
	std::span<std::string> args) {
	pid_t pid = fork();

	if (pid == -1) {
		throw std::system_error(errno, std::system_category());
	}

	if (pid == 0) {
		// we're in the child process
		auto path_native = name.c_str();
		auto exec_args = to_args_for_exec(args);
		exec_args.insert(exec_args.begin(), const_cast<char *>(name.c_str()));

		if (name.is_absolute()) {
			execv(path_native, exec_args.data());
		} else {
			execvp(path_native, exec_args.data());
		}

		// exec should never return on success, so it has failed here
		throw std::system_error(errno, std::system_category());
	}

	return Process {pid};
}
int Process::wait_for() {

	int proc_status;
	int wait_result = waitpid(pid_, &proc_status, 0);
	if (wait_result != pid_) {
		// failed to execute child
		throw std::system_error(errno, std::generic_category());
	}

	pid_ = NO_PID;
	int exitcode = WEXITSTATUS(proc_status);
	return exitcode;
}

//////////////////////////////////////////////////
/// Compiler
/// GnuCompatibleCompiler
/// ClangCompiler
/// GnuCompiler
//////////////////////////////////////////////////

std::vector<std::string> GnuCompatibleCompiler::flags(
	const CompilerOpts& opts) {
	std::vector<std::string> options;

	// Standard
	options.push_back(std::format("-std=c++{}", opts.std));

	// Warnings
	if (opts.wall) {
		options.push_back("-Wall");
	}

	if (opts.wextra) {
		options.push_back("-Wextra");
	}

	if (opts.wpedantic) {
		options.push_back("-pedantic");
	}

	// Debug info
	if (opts.debug_info) {
		options.push_back("-g");
	}

	// Sanitizer
	if (opts.sanitize_address) {
		options.push_back("-fsanitize=address");
	}

	return options;
}

std::vector<std::string> GnuCompatibleCompiler::out_file_flag(
	const std::filesystem::path& out_file_path) {
	std::vector<std::string> options;
	options.push_back("-o");
	options.push_back(out_file_path.string());
	return options;
}

static std::vector<std::string> split_by(char delimeter, std::string_view str) {
	using namespace boost::algorithm;
	std::vector<std::string> items;
	split(items, str, [delimeter](char ch) { return ch == delimeter; });
	return items;
}

/**
 * @brief Searches the PATH environment variable for `file`.
 * If the file is found, returns its full path,
 * otherwise returns an empty path.
 * @param file the file to search for
 * @return the full path of the file, or an empty path
 */
static std::filesystem::path search_path_variable(
	const std::filesystem::path& file) {
	auto PATH = std::getenv("PATH");
	if (PATH == nullptr) {
		return {};
	}

	using namespace std::filesystem;

	auto directories = split_by(':', PATH);
	for (auto& dir : directories) {
		if (dir.empty()) {
			continue;
		}

		auto found = dir / file;
		if (exists(found)) {
			return found;
		}
	}

	return {};
}

std::unique_ptr<ClangCompiler> ClangCompiler::find() {
	auto path = search_path_variable("clang++");
	if (path.empty()) {
		return {};
	} else {
		return std::unique_ptr<ClangCompiler> {new ClangCompiler {path}};
	}
}

std::unique_ptr<GnuCompiler> GnuCompiler::find() {
	auto path = search_path_variable("g++");
	if (path.empty()) {
		return {};
	} else {
		return std::unique_ptr<GnuCompiler> {new GnuCompiler {path}};
	}
}

std::unique_ptr<Compiler> Compiler::from_path(
	const std::filesystem::path& path) {
	if (path.filename() == "clang++" || path.filename() == "g++") {
		return std::unique_ptr<GnuCompatibleCompiler> {
			new GnuCompatibleCompiler {path}};
	} else {
		return {};
	}
}

std::unique_ptr<Compiler> Compiler::get() {
	auto CXX = getenv("CXX");

	if (CXX != nullptr) {
		auto compiler_path = search_path_variable(CXX);
		if (!compiler_path.empty()) {
			return Compiler::from_path(compiler_path);
		}
	}

	if (auto clang = ClangCompiler::find(); clang != nullptr) {
		return clang;
	} else if (auto gxx = GnuCompiler::find(); gxx != nullptr) {
		return gxx;
	} else {
		return {};
	}
}

bool Compiler::compile(const CompilerOpts& opts,
	const std::filesystem::path& out_file,
	const std::filesystem::path& in_file) {

	auto args = flags(opts);
	args.push_back("-c");
	args.append_range(out_file_flag(out_file));
	args.push_back(in_file);

	auto process = Process::start(path_, args);
	return process.wait_for() == 0;
}
} // namespace freight