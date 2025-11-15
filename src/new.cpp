#include <cassert>
#include <filesystem>
#include <fstream>
#include <optional>
#include <print>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "io.h"
#include "manifest.h"

namespace freight {
	struct MakeOptions {
		std::filesystem::path path;
		std::string name;
		std::string standard;
		std::string version;
	};

	bool make(const MakeOptions& opts) {
		namespace fs = std::filesystem;

		if (!fs::create_directories(opts.path)) {
			std::println(stderr,
				"failed to create project `{}` at `{}`",
				opts.name,
				opts.path.string());
			return false;
		}

		Manifest manifest;
		auto manifest_path = opts.path / Manifest::FILENAME;

		assert(!fs::exists(manifest_path));

		manifest.name = opts.name;
		manifest.standard = opts.standard;
		manifest.version = opts.version;

		std::ofstream stream {manifest_path};
		if (!stream.is_open()) {
			err::fail(std::format("failed to create manifest at `{}`",
						  manifest_path.string()))
				.cause("could not open file");
		}

		stream << "[project]\n";
		stream << std::format("name = \"{}\"\n", manifest.name);
		stream << std::format("version = \"{}\"\n", manifest.version);
		stream << std::format("standard = \"{}\"\n", manifest.standard);

		auto src = opts.path / "src";
		fs::create_directory(src);

		auto main_file_path = src / "main.cpp";
		if (!fs::exists(main_file_path)) {
			std::ofstream stream {main_file_path};
			if (stream.is_open()) {
				std::println(stderr,
					"error: failed to create `{}`",
					main_file_path.string());
				return false;
			}

			static const std::string DEFAULT_MAIN_SOURCE_FILE_TEXT =
				"#include <iostream>\n"
				"\n"
				"int main() {\n"
				"    std::cout << \"Hello, world!\";\n"
				"}\n";

			stream << DEFAULT_MAIN_SOURCE_FILE_TEXT;
		}

		return true;
	}

	struct NewOptions {
		std::filesystem::path path;
		std::string name;
		std::optional<std::string> standard;
		std::optional<std::string> version;
	};

	bool new_(const NewOptions& opts) {
		namespace fs = std::filesystem;

		std::println("    Creating binary project `{}`", opts.name);

		if (fs::exists(opts.path)) {
			err::fail(std::format(
				"destination `{}` already exists", opts.path.string()));
		}

		static const std::string DEFAULT_PROJECT_VERSION = "0.1.0";
		static const std::string DEFAULT_CXX_STANDARD = "c++20";

		MakeOptions makeopts {
			opts.path,
			opts.name,
			opts.standard.value_or(DEFAULT_CXX_STANDARD),
			opts.version.value_or(DEFAULT_PROJECT_VERSION),
		};

		return make(makeopts);
	}
} // namespace freight