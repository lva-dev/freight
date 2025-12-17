#include "pch.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "cmds.h"
#include "compiler.h"
#include "io.h"
#include "manifest.h"
#include "toml++/toml.hpp"

using namespace freight;
using namespace freight::io;
using freight::err::fail;

static err::Failure fail_parsing_manifest(const std::filesystem::path& file) {
	using namespace std::filesystem;
	return fail("failed to parse manifest at `{}`", absolute(file).string());
}

static std::string parse_name(const toml::node_view<toml::node>& project_table,
	const std::filesystem::path& manifest_path) {
	auto name_node = project_table["name"];
	if (!name_node) {
		fail_parsing_manifest(manifest_path)
			.cause("missing field `project.name`")
			.exit();
	}

	if (!name_node.is_string()) {
		fail("`project.name`: invalid type: expected a string").exit();
	}

	auto name = name_node.ref<std::string>();

	if (name.empty()) {
		fail("`project.name`: project name cannot be empty").exit();
	}

	if (std::isdigit(name[0])) {
		fail(
			"`project.name`: invalid character `{}` in project "
			"name: `{}`, the name cannot start with a digit",
			name[0],
			name)
			.exit();
	}

	return name;
}

static bool is_standard_valid(std::string_view str) {
	static const std::unordered_set<std::string_view> STANDARDS = {
		"03", "11", "14", "17", "20", "23"};
	return STANDARDS.contains(str);
}

static std::string parse_standard(
	const toml::node_view<toml::node>& project_table,
	const std::filesystem::path& manifest_path) {
	auto standard_node = project_table["standard"];

	if (!standard_node) {
		warning("no standard set: defaulting to 20 while the latest is 23");

		return "20";
	}

	if (!standard_node.is_integer()) {
		fail("`project.standard`: invalid type: expected an integer").exit();
	}

	auto std = standard_node.ref<std::string>();
	if (!is_standard_valid(std)) {
		fail_parsing_manifest(manifest_path)
			.cause("failed to parse the `standard` key")
			.cause(std::format("reccommended standard values are `17`, "
							   "`20`, or `23` but `{}` is unknown",
				std))
			.exit();
	}

	return std;
}

static std::string parse_version(
	const toml::node_view<toml::node>& project_table,
	[[maybe_unused]] const std::filesystem::path& manifest_path) {
	auto version_node = project_table["version"];
	if (!version_node) {
		return "0.0.0";
	} else {
		if (!version_node.is_string()) {
			fail("`project.version`: invalid type: expected a string").exit();
		}

		return version_node.ref<std::string>();
	}
}

static Manifest parse_manifest(const std::filesystem::path& path) {
	toml::table tbl;
	try {
		tbl = toml::parse_file(path.native());
	} catch (const toml::parse_error& err) {
		fail_parsing_manifest(path).cause(err.what()).exit();
	}

	auto project_table = tbl["project"];
	if (!project_table) {
		fail_parsing_manifest(path)
			.cause("manifest is missing [project]")
			.exit();
	}

	Manifest manifest;

	manifest.name = parse_name(project_table, path);
	manifest.standard = parse_standard(project_table, path);
	manifest.version = parse_version(project_table, path);

	return manifest;
}

static Manifest load_manifest(const std::filesystem::path& dir) {
	using namespace std::filesystem;

	auto manifest_path = absolute(dir / MANIFEST_FILENAME);
	if (!exists(manifest_path)) {
		fail("could not find `{}` in `{}`",
			MANIFEST_FILENAME,
			absolute(dir.string()).string())
			.exit();
	}

	if (!exists(dir / "src")) {
		fail_parsing_manifest(manifest_path)
			.cause("no target available\ndirectory src must be present")
			.exit();
	}

	return parse_manifest(manifest_path);
}


std::vector<std::filesystem::path> compile_source_files(const CompilerOpts& opts,
	Compiler& compiler,
	const std::filesystem::path& src_dir,
	const std::filesystem::path& out_dir) {
	using namespace std::filesystem;

	// compile source files
	std::vector<path> obj_files;
	bool all_successful = true;
	auto src_it = recursive_directory_iterator {src_dir};
	for (auto src_file : src_it) {
		auto obj_file = out_dir / relative(src_file.path() / ".o");
		obj_files.push_back(obj_file);
		bool success = compiler.compile(opts, obj_file, src_file);
		all_successful = all_successful && success;
	}

	if (!all_successful) {
		fail("failed to build files").exit();
	}

	return obj_files;
}

void build_all(const BuildOptions& opts, const Manifest& manifest) {
	using namespace std::filesystem;

	auto compiler = freight::Compiler::get();
	if (!compiler) {
		fail("failed to build files")
			.cause("could not locate a C++ compiler")
			.exit();
	}

	path target_dir = opts.path / "target";
	path int_dir = target_dir / "build";
	path src_dir = opts.path / "src";
    
	CompilerOpts compiler_opts {.std = manifest.standard};
    
	auto obj_files = compile_source_files(compiler_opts, *compiler, src_dir, int_dir);
    path exe_path = target_dir / manifest.name;
    
	bool linked = compiler->link(compiler_opts, exe_path, obj_files);
    if (!linked) {
		fail("failed to build files")
			.cause("linker error")
			.exit();
	}
}

void build(const BuildOptions& opts) {
	Manifest manifest = load_manifest(opts.path);
	build_all(opts, manifest);
}
