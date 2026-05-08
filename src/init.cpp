#include "pch.h"

#include "cmds.h"
#include "util.h"
#include "workspace.h"

static std::string cause_failed_to_create_dir(const std::filesystem::path& dir,
	const std::error_code& err) {
	return cause("failed to create directory {}", dir.string()) + "\n\n" +
		   cause("{} (os error {})", err.message(), err.value());
}

static void create_directory_or_bail(const std::filesystem::path dir,
	const std::string package_name) {
	std::error_code err;
	std::filesystem::create_directory(dir, err);
	if (err) {
		bail("failed to create package `{}` at `{}`\n\n{}",
			package_name,
			dir.string(),
			cause_failed_to_create_dir(dir, err));
	}
}

static bool create_manifest_file(const Package& package) {
	auto text = std::format(
		"[package]\n"
		"name = \"{}\"\n"
		"version = \"0.1.0\"\n"
		"standard = \"c23\"",
		package.name());

	return io::write_file(package.manifest_path(), text);
}

static bool create_main_cpp_file(const Package& package) {
	auto text = std::format(
		"#include <iostream>\n"
		"\n"
		"int main() {{\n"
		"    std::cout << \"Hello, world!\\n\";    \n"
		"}}\n");

	return io::write_file(package.root() / "src/main.cpp", text);
}

static void initialize_package(const Package& package) {
	if (package.name().empty()) {
		bail("package name cannot be empty");
	}

	std::error_code err;
	std::filesystem::create_directory(package.root() / "src", err);
	if (err) {
		bail("Failed to create package\n\n%s", cause(strerror(errno)));
	}

	create_manifest_file(package);
	create_main_cpp_file(package);
}

static Manifest get_default_manifest([[maybe_unused]] const GlobalContext& gctx,
	const std::filesystem::path& package_dir) {
	std::string package_name = package_dir.string();
	std::vector<Target> targets = {Target {.name = package_name, .paths = {"src"}}};
	return {{}, package_name, std::move(targets), Standard::CXX23};
}

static bool has_manifest(const std::filesystem::path& dir) {
	auto manifest_path = dir / "Stim.toml";
	return std::filesystem::exists(manifest_path);
}

void exec_init(const ds::Strings& args) {
	using namespace std::filesystem;

	status("Creating", "binary (application) package");

	path cwd;
	if (args.size() == 0) {
		cwd = current_path();
	} else {
		path given_path = args[0];
		cwd = given_path.is_absolute() ? given_path : current_path() / given_path;
	}

	GlobalContext gctx {cwd};
	std::string name = cwd.filename();

	if (has_manifest(cwd)) {
		bail("`stim init` cannot be run on existing Stim packages");
	}

	create_directory_or_bail(cwd, name);

	Manifest manifest = get_default_manifest(gctx, cwd);
	Package package {std::move(manifest), cwd / "Stim.toml"};
	initialize_package(package);
}

void exec_new(const ds::Strings& args) {
	using namespace std::filesystem;

	if (args.size() < 1) {
		bail("missing required argument: <PATH>");
	}

	path given_path = (args)[0];
	auto cwd = given_path.is_absolute() ? given_path : current_path() / given_path;
	std::string name = cwd.filename();

	status("Creating", "binary (application) `{}` package", name);

	if (exists(cwd)) {
		bail(
			"destination `{}` already exists\n\n"
			"Use `stim init` to initialize the directory",
			cwd.string());
	}

	create_directory_or_bail(cwd, cwd.filename());

	GlobalContext gctx {cwd};
	Manifest manifest = get_default_manifest(gctx, cwd);
	Package package {std::move(manifest), cwd / "Stim.toml"};
	initialize_package(package);
}
