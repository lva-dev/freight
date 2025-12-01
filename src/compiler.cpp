#include "pch.h"

#include "compiler.h"

#include <exception>
#include <ranges>
#include <stdexcept>
#include <utility>

//////////////////////////////////////////////////
/// freight::Process
//////////////////////////////////////////////////

freight::Process::~Process() {
    if (_pid != NO_PID) {
        std::terminate();
    }
}

static std::vector<char *> to_args_for_exec(const std::vector<std::string>& args) {
	using namespace std::ranges;
	using namespace std::ranges::views;
    auto filtered = transform(args, [](const std::string& str) { return const_cast<char *>(str.data()); });
	std::vector<char *> c_args {filtered.begin(), filtered.end()};
	c_args.push_back(nullptr);
	return c_args;
}

freight::Process freight::Process::start(const std::filesystem::path& name,
	std::vector<std::string>& args) {
	pid_t pid = fork();

	if (pid == -1) {
		throw std::system_error(errno, std::generic_category());
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
		throw std::system_error(errno, std::generic_category());
	}

	return Process {pid};
}

int freight::Process::wait_for() {
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

//////////////////////////////////////////////////
/// freight::Compiler
/// freight::GnuCompatibleCompiler
/// freight::ClangCompiler
/// freight::GnuCompiler
//////////////////////////////////////////////////

std::vector<std::string> freight::GnuCompatibleCompiler::build_options(
	const CompilerOpts& opts) {
	std::vector<std::string> options;

	// Standard
	options.push_back(std::format("-std=c++", opts.std));

	// Warnings
	if (opts.warnings.all)
		options.push_back("-Wall");

	if (opts.warnings.extra)
		options.push_back("-Wextra");

	if (opts.warnings.pedantic)
		options.push_back("-pedantic");

	// Debug info
	if (opts.debug_info)
		options.push_back("-g");

	// Sanitizer
	if (opts.sanitizer.address)
		options.push_back("-fsanitize=address");

	return options;
}

std::vector<std::string> freight::GnuCompatibleCompiler::build_out_file_option(
	const std::filesystem::path& out_file_path) {
	std::vector<std::string> options;
	options.push_back("-o");
	options.push_back(out_file_path.string());
	return options;
}

/**
 * @brief Searches the PATH environment variable for `file`.
 * If the file is found, returns its full path,
 * otherwise returns an empty path.
 * @param file the file to search for
 * @return the full path of the file, or an empty path
 */
static std::filesystem::path search_path_variable(std::filesystem::path file) {
	auto PATH = std::getenv("PATH");
	if (PATH == nullptr) {
		return {};
	}

	std::filesystem::path dir;
	for (char ch : std::string_view {PATH}) {
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
	if (path.empty()) {
		return {};
	} else {
		return std::unique_ptr<ClangCompiler> {new ClangCompiler {path}};
	}
}

std::unique_ptr<freight::GnuCompiler> freight::GnuCompiler::find() {
	auto path = search_path_variable("g++");
	if (path.empty()) {
		return {};
	} else {
		return std::unique_ptr<GnuCompiler> {new GnuCompiler {path}};
	}
}

std::unique_ptr<freight::Compiler> freight::Compiler::from_path(
	const std::filesystem::path& path) {
	if (path == "clang++" || path == "g++") {
		return std::unique_ptr<GnuCompatibleCompiler> {
			new GnuCompatibleCompiler {path}};
	} else {
		return {};
	}
}

std::unique_ptr<freight::Compiler> freight::Compiler::get() {
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

bool freight::Compiler::compile(const CompilerOpts& opts,
	const std::filesystem::path& out_file,
	const std::filesystem::path& in_file) {
	using namespace std::filesystem;

	// add options/arguments
	auto args = build_options(opts);
	args.append_range(build_out_file_option(out_file));
	args.push_back(in_file);

	// invoke compiler
	auto process = Process::start(_path, args);
	return process.wait_for() != 0;
}
