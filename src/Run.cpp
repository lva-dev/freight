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
	DebugInfo debugLevel;
	OptLevel optLevel;
	Standard standard;
};

struct Unit
{
	const Package *package;
	const Target *target;
	const Profile *profile;
};

struct Build
{
	const GlobalContext *gctx;
	const Workspace *workspace;
	std::vector<Unit> roots;
};

class Linker
{
private:
	const Build *ctx;
	std::vector<std::filesystem::path> files;
public:
	Linker(const Build& ctx) : ctx {&ctx}
	{}

	void add_object(const std::filesystem::path& unit);
	auto link(const std::filesystem::path exe) -> bool;
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

auto Linker::link(const std::filesystem::path exe) -> bool
{
	using namespace std::filesystem;

	create_directories(exe.parent_path());

	auto process = support::process::ProcessBuilder {ctx->gctx->compiler_path()};
	process.args(files);
	process.arg("-o");
	process.arg(exe);

	auto linkResult = process.status();
	if (!linkResult.has_value())
	{
		error::bail("{}",
			could_not_execute_process(process.get_path().native(), linkResult.error()));
	}

	if (linkResult->exit_code() != 0)
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
	else if (level == OptLevel::LevelS)
	{
		return 's';
	}
	else if (level == OptLevel::LevelZ)
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
	for (auto& path : paths)
	{
		if (is_regular_file(path))
		{
			files.push_back(path);
		}
		else if (is_directory(path))
		{
			for (auto& file : recursive_directory_iterator {path})
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

auto static compile_unit(const Build& ctx,
	const Unit& unit,
	const CompileOptions& opts) -> std::optional<std::filesystem::path>
{
	using namespace std::filesystem;

	support::process::ProcessBuilder compilerBase {ctx.gctx->compiler_path()};

	compilerBase.arg("-c");

	if (opts.debugLevel != DebugInfo::Level0)
	{
		compilerBase.arg(std::format("-g{}", debuglevel_to_int(opts.debugLevel)));
	}

	if (opts.optLevel != OptLevel::Level0)
	{
		compilerBase.arg(std::format("-O{}", optlevel_to_char(opts.optLevel)));
	}

	compilerBase.arg(std::format("-std={}", standard_to_str(opts.standard)));

	std::vector<support::io::AnonymousFile> objectFiles;

	bool hadError = false;
	for (auto& sourceFile : expand_linear_paths(unit.target->paths))
	{
		support::process::ProcessBuilder compiler {compilerBase};

		compiler.arg(sourceFile);

		auto objectFile = support::io::AnonymousFile::create();
		if (!objectFile.has_value())
		{
			error::bail("failed to build package");
		}

		compiler.arg("-o");
		compiler.arg(objectFile->path());

		auto compilerResult = compiler.status();
		if (!compilerResult.has_value())
		{
			error::bail("{}",
				could_not_execute_process(
					compiler.get_path().native(), compilerResult.error()));
		}

		if (compilerResult->exit_code() != 0)
		{
			hadError = true;
		}
		else
		{
			objectFiles.emplace_back(std::move(*objectFile));
		}
	}

	if (hadError)
	{
		std::string binDescription =
			ctx.roots.size() == 1 ? std::format("(bin \"{}\")", unit.target->name) : "";
		error::print_error("could not compile `{}` {} due to error(s)",
			unit.package->name(),
			binDescription);
		return {};
	}

	Linker linker {ctx};
	for (const auto& file : objectFiles)
	{
		linker.add_object(file.path());
	}

	auto binaryPath =
		ctx.workspace->build_dir() / unit.profile->target_subdir / unit.target->name;
	if (!linker.link(binaryPath))
	{
		error::print_error("could not compile `{}` (bin \"{}\") due to linker error(s)",
			unit.package->name(),
			unit.target->name);
		return {};
	}

	return binaryPath;
}

struct CompileResult
{
	std::vector<std::filesystem::path> binaries;
};

static auto compile(const Build& ctx, const CompileOptions& opts) -> CompileResult
{
	CompileResult compilation;

	for (auto& unit : ctx.roots)
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
static auto to_milliseconds(std::chrono::duration<R, P> d) -> float
{
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
	constexpr static const double MILLISECONDS_PER_SECOND = 1000;
	return static_cast<double>(duration_cast<milliseconds>(d).count()) /
		   MILLISECONDS_PER_SECOND;
}

static auto build_package(const Workspace& ws,
	const Package& package,
	std::vector<std::string> targetsToBuild = {}) -> CompileResult
{
	using std::chrono::steady_clock;

	error::print_status("Compiling", "{} ({})", package.name(), package.root().native());

	auto startTime = steady_clock::now();

	Build bctx {.gctx = &ws.gctx(), .workspace = &ws, .roots = {}};

	Profile profile = Profile::dev();

	for (auto& target : package.targets())
	{
		if (!targetsToBuild.empty() &&
			!std::ranges::contains(targetsToBuild, target.name))
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
		.debugLevel = profile.debug,
		.optLevel = profile.optLevel,
		.standard = package.standard(),
	};

	CompileResult result = compile(bctx, opts);

	auto endTime = steady_clock::now();
	auto timePassed = endTime - startTime;

	std::string description =
		opts.optLevel == OptLevel::Level0 ? "unoptimized" : "optimized";

	if (opts.debugLevel != DebugInfo::Level0)
	{
		description += " + debuginfo";
	}

	error::print_status(" Finished",
		"`{}` profile [{}] target(s) in {:.3}s",
		profile.name,
		description,
		to_milliseconds(timePassed));

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

void ops::build(GlobalContext& gctx, const ops::BuildOptions&)
{
	using namespace std::filesystem;

	auto ws = Workspace::open(gctx.cwd() / "Freight.toml", gctx);
	build_package(ws, ws.current());
}

void ops::run(GlobalContext& gctx,
	const ops::RunOptions&,
	[[maybe_unused]] std::span<std::string> args)
{
	using namespace std::filesystem;

	auto ws = Workspace::open(gctx.cwd() / "Freight.toml", gctx);

	assert(!ws.current().targets().empty());

	CompileResult result;
	if (ws.current().targets().size() == 1)
	{
		result = build_package(ws, ws.current());
	}
	else
	{
		// TODO: Implement target selection logic for multiple-target projects.
		// For now, assert false
		// compiliation = build_package(ws, ws.current(), /* targets to run */);
		assert(false);
	}

	auto exePath = result.binaries.front();
	auto exePathRelative = relative(result.binaries.front(), gctx.cwd());
	error::print_status("  Running", "`{}`", exePathRelative.native());

	support::process::ProcessBuilder pb {exePath};
	auto runResult = pb.status();
	if (!runResult.has_value())
	{
	}

	exit(runResult->exit_code());
}