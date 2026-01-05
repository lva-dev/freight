#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

#include "cmds.h"
#include "io.h"
#include "manifest.h"

using freight::err::fail;
using namespace freight::io;

namespace freight {
struct MakeOptions {
	std::filesystem::path path;
	std::string name;
	std::string standard;
};

static void create_manifest_file(const MakeOptions& opts) {
	using namespace std::filesystem;

	auto manifest_path = opts.path / MANIFEST_FILENAME;

	// manifest file shouldn't exist for either 'new' or 'init'
	assert(!exists(manifest_path));

	std::ofstream stream {manifest_path};
	if (!stream.is_open()) {
		fail("failed to create manifest at `{}`",
			absolute(manifest_path).string())
			.cause("could not open file")
			.exit();
	}

	static const std::string DEFAULT_PROJECT_VERSION = "0.1.0";

	stream << "[project]\n";
	stream << std::format("name = \"{}\"\n", opts.name);
	stream << std::format("version = \"{}\"\n", DEFAULT_PROJECT_VERSION);
	stream << std::format("standard = \"{}\"\n", opts.standard);
}

static void create_source_files(const MakeOptions& opts) {
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

	// TODO: assert that standard is valid

	if (std::stod(opts.standard) >= 23) {
		stream << MAIN_BINARY_SOURCE_POST_23;
	} else {
		stream << MAIN_BINARY_SOURCE_PRE_23;
	}
}

static void make(const MakeOptions& opts) {
	create_manifest_file(opts);
	create_source_files(opts);
}

static const std::string DEFAULT_CXX_STANDARD = "20";

static inline bool try_create_directories(const std::filesystem::path& path) {
	using namespace std::filesystem;

	std::error_code errc;
	bool result = create_directories(path);
	if (errc) {
		fail("failed to create project at `{}`", path.string())
			.cause(errc.message())
			.exit();
	}

	return result;
}

void new_([[maybe_unused]] GlobalContext& gctx, const NewOptions& opts) {
	using namespace std::filesystem;

	auto& path = opts.path;
	auto name = opts.name.value_or(opts.path.filename());
	println("    \033[32mCreating\033[m binary `{}` project", name);

	if (exists(opts.path)) {
		err::destination_exists(path)
			.msg("Use `freight init` to initialize the directory")
			.exit();
	}

	try_create_directories(opts.path);

	make({
		path,
		name,
		opts.standard.value_or(DEFAULT_CXX_STANDARD),
	});
}

void init(GlobalContext& gctx, const InitOptions& opts) {
	using namespace std::filesystem;

	println("    \033[32mCreating\033[m binary project");

	auto path = opts.path.value_or(gctx.cwd());
	make({
		path,
		opts.name.value_or(path.filename()),
		opts.standard.value_or(DEFAULT_CXX_STANDARD),
	});
}
} // namespace freight