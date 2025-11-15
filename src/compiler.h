#pragma once

#include <filesystem>
#include <memory>
#include <vector>

namespace freight {
	struct CompileOpts {
	public:
		std::string std = "c++20";
		std::filesystem::path output_dir = std::filesystem::current_path();
		bool debug_info = false;

		struct {
			bool link = false;
			bool comp = false;
		} action {};

		struct {
			bool all = false;
			bool extra = false;
			bool pedantic = false;
		} warnings {};

		struct {
			bool address = false;
		} sanitizer {};
	};

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
	public:
		/**
		 * @brief Returns a compiler object for a C++ compiler on this system.
		 * If a compiler couldn't be found, returns a null pointer.
		 * On POSIX systems, first searches for clang++, then g++.
		 * @return a compiler on this system
		 */
		static std::unique_ptr<Compiler> get();

		virtual ~Compiler() = default;
		Compiler() = delete;
		Compiler(const Compiler&) = delete;
		Compiler(Compiler&&) = default;
		Compiler& operator=(const Compiler&) = delete;
		Compiler& operator=(Compiler&&) = default;

		virtual bool compile(const CompileOpts& opts,
			std::filesystem::path in_file) = 0;
	};

	class GnuCompatibleCompiler : public Compiler {
	protected:
		using Compiler::Compiler;
	private:
		std::vector<std::string> build_args(const CompileOpts& opts);
	public:
		virtual bool compile(const CompileOpts& opts,
			std::filesystem::path in_file) override;
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
} // namespace freight