#include "Pch.hpp"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <system_error>
#include <vector>

#include "Error.hpp"
#include "Ops.hpp"
#include "Workspace.hpp"
#include "Support/Io.hpp"
#include "Support/Process.hpp"

using namespace freight;

struct CompileOptions
{
	DebugInfo debug_level;
	OptLevel opt_level;
	Standard standard;
};

struct Unit
{
	const Package *package;
	const Target *target;
	const Profile *profile;
};

struct BuildContext
{
	const GlobalContext *gctx;
	const Workspace *workspace;
	std::vector<Unit> roots;
};

struct BuildRunner
{
    const BuildContext *bcx;

    
};

class Linker
{
private:
	const BuildContext *ctx;
	std::vector<std::filesystem::path> files;
public:
	Linker(const BuildContext& ctx) : ctx {&ctx}
	{}

	void add_object(const std::filesystem::path& unit);
	auto link(const std::filesystem::path& exe) -> bool;
};

void Linker::add_object(const std::filesystem::path& unit)
{
	files.push_back(unit);
}

static auto could_not_execute_process(std::string_view process,
	const std::error_code& err) -> std::string
{

	return std::format("{}\n\n{}",
		error::cause("could not execute process `{}` (never executed)", process),
		error::cause("{} (os error {})", err.message(), err.value()));
}

auto Linker::link(const std::filesystem::path& exe) -> bool
{
	using namespace std::filesystem;

	create_directories(exe.parent_path());

	auto process = support::process::ProcessBuilder {ctx->gctx->compiler_path()};
	process.args(files);
	process.arg("-o");
	process.arg(exe);

	auto link_result = process.status();
	if (!link_result.has_value())
	{
		error::bail(
			could_not_execute_process(process.get_path().native(), link_result.error()));
	}

	if (link_result->exit_code() != 0)
	{
		return false;
	}

	return true;
}

static auto optlevel_to_char(OptLevel level) -> char
{
	if (level <= OptLevel::Level3)
	{
		return static_cast<char>('0' + static_cast<int>(level));
	}
	
    if (level == OptLevel::LevelS)
	{
		return 's';
	}
	
    if (level == OptLevel::LevelZ)
	{
		return 'z';
	}

	std::unreachable();
}

static auto debuglevel_to_int(DebugInfo level) -> int
{
	return static_cast<int>(level);
}

static auto standard_to_str(Standard standard) -> std::string
{
	switch (standard)
	{
	case Standard::Cpp23:
		return "c++23";
	}
}

static auto expand_linear_paths(
	std::span<const std::filesystem::path> paths) -> std::vector<std::filesystem::path>
{
	using namespace std::filesystem;

	std::vector<std::filesystem::path> files;
	for (const auto& path : paths)
	{
		if (is_regular_file(path))
		{
			files.push_back(path);
		}
		else if (is_directory(path))
		{
			for (const auto& file : recursive_directory_iterator {path})
			{
				files.push_back(file);
			}
		}
		else
		{
			assert(false && "expand_paths can only expand regular files and directories");
		}
	}

	return files;
}

auto static compile_unit(const BuildContext& ctx,
	const Unit& unit,
	const CompileOptions& opts) -> std::optional<std::filesystem::path>
{
	using namespace std::filesystem;

	support::process::ProcessBuilder compiler_base {ctx.gctx->compiler_path()};

	compiler_base.arg("-c");

	if (opts.debug_level != DebugInfo::Level0)
	{
		compiler_base.arg(std::format("-g{}", debuglevel_to_int(opts.debug_level)));
	}

	if (opts.opt_level != OptLevel::Level0)
	{
		compiler_base.arg(std::format("-O{}", optlevel_to_char(opts.opt_level)));
	}

	compiler_base.arg(std::format("-std={}", standard_to_str(opts.standard)));

	std::vector<support::io::AnonymousFile> object_files;

	bool had_error = false;
	for (auto& source_file : expand_linear_paths(unit.target->paths))
	{
		support::process::ProcessBuilder compiler {compiler_base};

		compiler.arg(source_file);

		auto object_file = support::io::AnonymousFile::create();
		if (!object_file.has_value())
		{
			error::bail("failed to build package");
		}

		compiler.arg("-o");
		compiler.arg(object_file->path());

		auto compiler_result = compiler.status();
		if (!compiler_result.has_value())
		{
			error::bail(
				could_not_execute_process(
					compiler.get_path().native(), compiler_result.error()));
		}

		if (compiler_result->exit_code() != 0)
		{
			had_error = true;
		}
		else
		{
			object_files.emplace_back(std::move(*object_file));
		}
	}

	if (had_error)
	{
		std::string bin_description =
			ctx.roots.size() == 1 ? std::format("(bin \"{}\")", unit.target->name) : "";
		error::print_error("could not compile `{}` {} due to error(s)",
			unit.package->name(),
			bin_description);
		return {};
	}

	Linker linker {ctx};
	for (const auto& file : object_files)
	{
		linker.add_object(file.path());
	}

	auto binary_path =
		ctx.workspace->build_dir() / unit.profile->target_subdir / unit.target->name;
	if (!linker.link(binary_path))
	{
		error::print_error("could not compile `{}` (bin \"{}\") due to linker error(s)",
			unit.package->name(),
			unit.target->name);
		return {};
	}

	return binary_path;
}

struct CompileResult
{
	std::vector<std::filesystem::path> binaries;
};

static auto compile(const BuildContext& ctx, const CompileOptions& opts) -> CompileResult
{
	CompileResult compilation;

	for (const auto& unit : ctx.roots)
	{
		auto result = compile_unit(ctx, unit, opts);
		if (result)
		{
			compilation.binaries.push_back(*result);
		}
	}

	return compilation;
}

template<class R, class P>
static auto to_milliseconds(std::chrono::duration<R, P> duration) -> float
{
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
	constexpr static const double MILLISECONDS_PER_SECOND = 1000;
	return static_cast<double>(duration_cast<milliseconds>(duration).count()) /
		   MILLISECONDS_PER_SECOND;
}

static auto build_package(const Workspace& workspace,
	const Package& package,
	std::vector<std::string> targets_to_build = {}) -> CompileResult
{
	using std::chrono::steady_clock;

	error::print_status("Compiling", "{} ({})", package.name(), package.root().native());

	auto start_time = steady_clock::now();

	BuildContext bctx {.gctx = &workspace.gctx(), .workspace = &workspace, .roots = {}};

	Profile profile = Profile::dev();

	for (const auto& target : package.targets())
	{
		if (!targets_to_build.empty() &&
			!std::ranges::contains(targets_to_build, target.name))
		{
			continue;
		}

		bctx.roots.emplace_back(Unit {
			.package = &package,
			.target = &target,
			.profile = &profile,
		});
	}

	CompileOptions opts = {
		.debug_level = profile.debug,
		.opt_level = profile.opt_level,
		.standard = package.standard(),
	};

	CompileResult result = compile(bctx, opts);

	auto end_time = steady_clock::now();
	auto time_passed = end_time - start_time;

	std::string description =
		opts.opt_level == OptLevel::Level0 ? "unoptimized" : "optimized";

	if (opts.debug_level != DebugInfo::Level0)
	{
		description += " + debuginfo";
	}

	error::print_status(" Finished",
		"`{}` profile [{}] target(s) in {:.3}s",
		profile.name,
		description,
		to_milliseconds(time_passed));

	return result;
}

// TODO: Implement target selection logic for multiple-target projects.
// static std::filesystem::path build_packages(const Workspace& ws) {
//     using std::chrono::steady_clock;

//     for (auto& package : ws.packages()) {
//         build_package(ws, package);
//     }

//     return binary;
// }

void ops::build(GlobalContext& gctx, [[maybe_unused]] const ops::BuildOptions& opts)
{
	using namespace std::filesystem;

	auto workspace = Workspace::open(gctx.cwd() / "Freight.toml", gctx);
	build_package(workspace, workspace.current());
}

void ops::run(GlobalContext& gctx,
	[[maybe_unused]] const ops::RunOptions& opts,
	[[maybe_unused]] std::span<std::string> args)
{
	using namespace std::filesystem;

	auto workspace = Workspace::open(gctx.cwd() / "Freight.toml", gctx);

	assert(!workspace.current().targets().empty());

	CompileResult result;
	if (workspace.current().targets().size() == 1)
	{
		result = build_package(workspace, workspace.current());
	}
	else
	{
		// TODO: Implement target selection logic for multiple-target projects.
		// For now, assert false
		// compiliation = build_package(ws, ws.current(), /* targets to run */);
		assert(false);
	}

	auto exe_path = result.binaries.front();
	auto exe_path_relative = relative(result.binaries.front(), gctx.cwd());
	error::print_status("  Running", "`{}`", exe_path_relative.native());

	support::process::ProcessBuilder process {exe_path};
	auto run_result = process.status();
	if (!run_result.has_value())
	{
	}

	exit(run_result->exit_code());
}