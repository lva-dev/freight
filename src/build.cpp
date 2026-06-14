#include "pch.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <toml++/toml.hpp>
#include <unordered_set>
#include <vector>

#include "cmds.h"
#include "compiler.h"
#include "io.h"
#include "manifest.h"

using namespace freight;
using namespace freight::io;
using freight::err::fail;

namespace freight {
struct SourceFile {
private:
	std::filesystem::path path_;
	std::filesystem::file_time_type last_mtime_;
public:
	SourceFile(const std::filesystem::path& path);

	const std::filesystem::path& path() const;
	operator const std::filesystem::path&() const;

	// TODO: implement
	std::filesystem::file_time_type last_modified() const;
};

struct ObjectFile {
private:
	std::filesystem::path path_;
	bool succeeded_;
public:
	ObjectFile(const std::filesystem::path& path, bool succeeded = true);

	const std::filesystem::path& path() const;
	operator const std::filesystem::path&() const;

	bool succeeded() const;
	bool failed() const;
};

struct CxxBuilder {
private:
	std::vector<SourceFile> src_files() const;
	ObjectFile compile_one(const std::filesystem::path& source_file) const;
	std::vector<ObjectFile> compile_all() const;
	void link_all(const std::filesystem::path& executable_path,
		const std::vector<ObjectFile>& object_files) const;
public:
	std::unique_ptr<Compiler> compiler;
	CompilerOpts opts;
	std::filesystem::path src;
	std::filesystem::path obj;

	void build(const std::filesystem::path& executable_path) const;
};

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

	if (!standard_node.is_string()) {
		fail("`project.standard`: invalid type: expected a string").exit();
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

inline SourceFile::SourceFile(const std::filesystem::path& path)
	: path_ {path} {
	using namespace std::filesystem;

	if (!is_regular_file(path)) {
		throw std::invalid_argument {
			std::format("failed to construct SourceFile: `{}` is not a file",
				path.string())};
	}
}

inline const std::filesystem::path& SourceFile::path() const {
	return path_;
}

inline SourceFile::operator const std::filesystem::path&() const {
	return path_;
}

inline std::filesystem::file_time_type SourceFile::last_modified() const {
	return last_mtime_;
}

inline ObjectFile::ObjectFile(const std::filesystem::path& path, bool succeeded)
	: path_ {path},
	  succeeded_ {succeeded} {
	if (!is_regular_file(path)) {
		throw std::invalid_argument {
			std::format("failed to construct ObjectFile: `{}` is not a file",
				path.string())};
	}
}

inline const std::filesystem::path& ObjectFile::path() const {
	return path_;
}

inline ObjectFile::operator const std::filesystem::path&() const {
	return path_;
}

inline bool ObjectFile::succeeded() const {
	return succeeded_;
}

inline bool ObjectFile::failed() const {
	return !succeeded_;
}

std::vector<SourceFile> CxxBuilder::src_files() const {
	using namespace std::filesystem;

	std::vector<SourceFile> source_files;
	for (path file : recursive_directory_iterator {src}) {
		source_files.emplace_back(file);
	}

	return source_files;
}

ObjectFile CxxBuilder::compile_one(
	const std::filesystem::path& src_file) const {
	using namespace std::filesystem;

	path src_file_rel = relative(src_file, src);
	path obj_file_rel = src_file_rel.replace_extension("o");
	auto obj_file_path = obj / src_file_rel;

	bool succeeded = compiler->compile(opts, obj_file_path, src_file);
	return {obj_file_path, succeeded};
}

std::vector<ObjectFile> CxxBuilder::compile_all() const {
	using namespace std::filesystem;

	std::vector<ObjectFile> obj_files;
	for (const path& src_file : src_files()) {
		obj_files.push_back(compile_one(src_file));
	}

	for (const ObjectFile& file : obj_files) {
		if (file.failed()) {
			fail("failed to build files").cause("compilation error(s)").exit();
		}
	}

	return obj_files;
}

void CxxBuilder::link_all(const std::filesystem::path& executable_path,
	const std::vector<ObjectFile>& object_files) const {

	using namespace std::filesystem;

	std::vector<path> obj_file_paths;
	for (const ObjectFile& obj_file : object_files) {
		obj_file_paths.push_back(obj_file.path());
	}

	bool succeeded = compiler->link(opts, executable_path, obj_file_paths);
	if (!succeeded) {
		fail("failed to build files").cause("linking error(s)").exit();
	}
}

void CxxBuilder::build(const std::filesystem::path& executable_path) const {
	auto obj_files = compile_all();
	link_all(executable_path, obj_files);
}

static void build_all(GlobalContext& gctx,
	const BuildOptions& opts,
	const Manifest& manifest) {
	using namespace std::filesystem;

	auto compiler = freight::Compiler::get();
	if (!compiler) {
		fail("failed to build files")
			.cause("could not locate a C++ compiler")
			.exit();
	}

	CompilerOpts compiler_opts {.std = manifest.standard};
	path src_dir = gctx.cwd() / "src";
	path config_name = (opts.release) ? "release" : "debug";
	path target_dir = gctx.cwd() / "target" / config_name;
	path obj_dir = target_dir / "obj";

	create_directories(target_dir);
	create_directories(obj_dir);

	CxxBuilder builder {.compiler = std::move(compiler),
		.opts = std::move(compiler_opts),
		.src = std::move(src_dir),
		.obj = std::move(obj_dir)};

	path exe_path = target_dir / manifest.name;
	builder.build(exe_path);
}

void build(GlobalContext& gctx, const BuildOptions& opts) {
	Manifest manifest = load_manifest(gctx.cwd());
	build_all(gctx, opts, manifest);
}
} // namespace freight

template<> struct std::hash<freight::SourceFile> {
	std::size_t operator()(const SourceFile& file) const noexcept {
		return std::hash<std::filesystem::path> {}(file.path());
	}
};