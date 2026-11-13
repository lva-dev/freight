#include "Pch.hpp"

#include "Error.hpp"
#include "Ops.hpp"
#include "Toml.hpp"
#include "Workspace.hpp"
#include "Support/Io.hpp"

using namespace freight;

static auto cause_failed_to_create_dir(const std::filesystem::path& dir,
	const std::error_code& err) -> std::string
{
	return error::cause("failed to create directory {}\n\n", dir.native()) +
		   error::cause("{} (os error {})", err.message(), err.value());
}

static void create_directory_or_bail(const std::filesystem::path dir,
	const std::string package_name)
{
	std::error_code err;
	std::filesystem::create_directory(dir, err);
	if (err)
	{
		error::bail("failed to create package `{}` at `{}`\n\n{}",
			package_name,
			dir.native(),
			cause_failed_to_create_dir(dir, err));
	}
}

static auto create_manifest_file(const Package& package) -> bool
{
	auto text = std::format(
		"[package]\n"
		"name = \"{}\"\n"
		"version = \"0.1.0\"\n"
		"standard = \"23\"",
		package.name());

	return support::io::write_file(package.manifest_path(), text);
}

static auto create_main_cpp_file(const Package& package) -> bool
{
	constexpr auto text =
		"#include <iostream>\n"
		"\n"
		"int main() {\n"
		"    std::cout << \"Hello, world!\\n\";\n"
		"}\n";

	return support::io::write_file(package.root() / "src/main.cpp", text);
}

static void initialize_package(const Package& package)
{
	if (package.name().empty())
	{
		error::bail("package name cannot be empty");
	}

	std::error_code err;
	std::filesystem::create_directory(package.root() / "src", err);
	if (err)
	{
		error::bail("Failed to create package\n\n%s", error::cause(strerror(errno)));
	}

	create_manifest_file(package);
	create_main_cpp_file(package);
}

static auto get_default_manifest([[maybe_unused]] const GlobalContext& gctx,
	const std::filesystem::path& package_dir) -> Manifest
{
	std::string package_name = package_dir.native();
	std::vector<Target> targets = {Target {.name = package_name, .paths = {"src"}}};
	return Manifest::create(
		TomlManifest {}, package_name, std::move(targets), Standard::Cpp23);
}

static auto has_manifest(const std::filesystem::path& dir) -> bool
{
	auto manifest_path = dir / "Freight.toml";
	return std::filesystem::exists(manifest_path);
}

void ops::init(GlobalContext& gctx, const ops::InitOptions& opts)
{
	error::print_status("Creating", "binary (application) package");

	std::filesystem::path cwd;
	if (opts.path.has_value())
	{
        std::filesystem::path given_path = *opts.path;
        cwd = given_path.is_absolute() ? given_path : gctx.cwd() / given_path;
    }
	else
	{
        cwd = gctx.cwd();
	}

	std::string name = cwd.filename();

	if (has_manifest(cwd))
	{
		error::bail("`freight init` cannot be run on existing Freight packages");
	}

	create_directory_or_bail(cwd, name);

	Manifest manifest = get_default_manifest(gctx, cwd);
	auto package = Package::create(std::move(manifest), cwd / "Freight.toml");
	initialize_package(package);
}

void ops::new_(GlobalContext& gctx, const ops::NewOptions& opts)
{
	using namespace std::filesystem;

	path given_path = opts.path;
	auto cwd = given_path.is_absolute() ? given_path : (gctx.cwd() / given_path);
	std::string name = cwd.filename();

	error::print_status("Creating", "binary (application) `{}` package", name);

	if (exists(cwd))
	{
		error::bail(
			"destination `{}` already exists\n\n"
			"Use `freight init` to initialize the directory",
			cwd.native());
	}

	create_directory_or_bail(cwd, cwd.filename());

	Manifest manifest = get_default_manifest(gctx, cwd);
	auto package = Package::create(std::move(manifest), cwd / "Freight.toml");
	initialize_package(package);
}
