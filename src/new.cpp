#include <cassert>
#include <filesystem>
#include <fstream>
#include <optional>
#include <print>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "commands.h"
#include "io.h"
#include "manifest.h"

using freight::err::fail;

namespace freight {
	struct MakeOptions {
		std::filesystem::path path;
		std::string name;
		std::string standard;
		std::string version;
	};

	void create_manifest_file(const MakeOptions& opts) {
		using namespace std::filesystem;

		Manifest manifest;
		auto manifest_path = opts.path / Manifest::FILENAME;

		// if the parent dir shouldn't exist, manifest file shouldn't exist
		assert(!exists(manifest_path));

		std::ofstream stream {manifest_path};
		if (!stream.is_open()) {
			fail("failed to create manifest at `{}`",
				absolute(manifest_path).string())
				.cause("could not open file")
				.exit();
		}

		stream << "[project]\n";
		stream << std::format("name = \"{}\"\n", opts.name);
		stream << std::format("version = \"{}\"\n", opts.version);
		stream << std::format("standard = \"{}\"\n", opts.standard);
	}

	void create_source_files(const MakeOptions& opts) {
		using namespace std::filesystem;

		auto src = opts.path / "src";
		create_directory(src);

		auto main_file_path = src / "main.cpp";

		if (exists(main_file_path)) {
			return;
		}

		std::ofstream stream {main_file_path};
		if (!stream.is_open()) {
			fail("failed to create `{}`", main_file_path.string()).exit();
		}

		static const std::string MAIN_BINARY_SOURCE_PRE_23 =
			"#include <iostream>\n"
			"\n"
			"int main() {\n"
			"    std::cout << \"Hello, world!\\n\";\n"
			"}\n";

		static const std::string MAIN_BINARY_SOURCE_POST_23 =
			"#include <print>\n"
			"\n"
			"int main() {\n"
			"    std::println(\"Hello, world!\");\n"
			"}\n";

		if (std::stod(opts.standard.substr(3)) >= 23) {
			stream << MAIN_BINARY_SOURCE_POST_23;
		} else {
			stream << MAIN_BINARY_SOURCE_PRE_23;
		}
	}

	bool make(const MakeOptions& opts) {
		using namespace std::filesystem;

		if (!create_directories(opts.path)) {
			fail("failed to create project `{}` at `{}`",
				absolute(opts.name).string(),
				opts.path.string())
				.exit();
		}

		create_manifest_file(opts);
		create_source_files(opts);

		return true;
	}

	bool new_(const NewOptions& opts) {
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