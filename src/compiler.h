#pragma once

#include <filesystem>
#include <memory>
#include <ranges>
#include <vector>

namespace freight {
	/**
	 * @brief Options to pass to C++ compilers when invoked through
	 * `Compiler.compile()` and `Compiler.link()`.
	 */
	struct CompilerOpts {
		std::string std = "c++20";

		struct {
			bool all = false;
			bool extra = false;
			bool pedantic = false;
		} warnings {};

		bool debug_info = false;

		struct {
			bool address = false;
		} sanitizer {};
	};

	/**
	 * @brief An interface for a C++ compiler.
	 *
	 */
	class Compiler {
	private:
		/**
		 * @brief Returns a compiler object for the executable specified by
		 * `path`. On POSIX systems, if `path` is not `"clang++"` or `"g++"`,
		 * returns a null pointer.
		 * @param path the path to the compiler
		 * @return std::unique_ptr<freight::Compiler>
		 */
		static std::unique_ptr<Compiler> from_path(
			const std::filesystem::path& path);
	protected:
		std::filesystem::path _path;

		Compiler(const std::filesystem::path& path) : _path {path} {}

		virtual std::vector<std::string> build_options(
			const CompilerOpts& opts) = 0;
		virtual std::vector<std::string> build_out_file_option(
			const std::filesystem::path& out_file) = 0;
	public:
		Compiler() = delete;
		Compiler(const Compiler&) = delete;
		Compiler(Compiler&&) = default;
		Compiler& operator=(const Compiler&) = delete;
		Compiler& operator=(Compiler&&) = default;
		virtual ~Compiler() = default;

		/**
		 * @brief Returns a compiler object for a C++ compiler on this system.
		 * If a compiler couldn't be found, returns a null pointer.
		 * On POSIX systems, first searches for clang++, then g++.
		 * @return a compiler on this system
		 */
		static std::unique_ptr<Compiler> get();

		/**
		 * @brief Compiles the given file to an object file.
		 * @param opts
		 * @param in_file
		 * @return true
		 * @return false
		 */
		bool compile(const CompilerOpts& opts,
			const std::filesystem::path& out_file,
			const std::filesystem::path& in_file);

		template<class Paths>
			requires std::ranges::range<std::filesystem::path>
		bool link(const CompilerOpts& opts,
			const std::filesystem::path& out_file,
			const Paths& in_files);
	};

	class GnuCompatibleCompiler : public Compiler {
	protected:
		using Compiler::Compiler;
		virtual std::vector<std::string> build_options(
			const CompilerOpts& opts) override;
		virtual std::vector<std::string> build_out_file_option(
			const std::filesystem::path& out_file) override;
	};

	class ClangCompiler : public GnuCompatibleCompiler {
	private:
		using GnuCompatibleCompiler::GnuCompatibleCompiler;
	public:
		/**
		 * @brief Returns a compiler object for clang++, if it is found on this
		 * system. Searches through the PATH environment variable for clang++.
		 * If it couldn't be found, returns a null pointer.
		 * @return std::unique_ptr<freight::ClangCompiler>
		 */
		static std::unique_ptr<ClangCompiler> find();
	};

	class GnuCompiler : public GnuCompatibleCompiler {
	private:
		using GnuCompatibleCompiler::GnuCompatibleCompiler;
	public:
		/**
		 * @brief Returns a compiler object for g++, if it is found on this
		 * system. Searches through the PATH environment variable for g++. If it
		 * couldn't be found, returns a null pointer.
		 * @return std::unique_ptr<freight::ClangCompiler>
		 */
		static std::unique_ptr<GnuCompiler> find();
	};

	/**
	 * @brief
	 *
	 */
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
} // namespace freight

template<class Paths>
	requires std::ranges::range<std::filesystem::path>
bool freight::Compiler::link(const CompilerOpts& opts,
	const std::filesystem::path& out_file,
	const Paths& in_files) {
	namespace fs = std::filesystem;

	// add options
	auto args = build_options(opts);
    args.append_range(build_out_file_option(out_file));

	// add source files
	auto cwd = fs::current_path();
	for (auto file : in_files) {
		args.push_back(cwd / file);
	}

	// invoke compiler
	auto process = Process::start(_path, args);
	return process.wait_for() != 0;
}
