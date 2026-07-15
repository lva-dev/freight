#include "Pch.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <sys/mman.h>
#include <vector>

#include "Cmds.h"
#include "Support/Io.h"
#include "Support/Util.h"
#include "Workspace.h"

static std::vector<std::filesystem::path> expand_linear_paths(
	std::span<const std::filesystem::path> paths)
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
	{
	}

	void add_object(const std::filesystem::path& unit);
	bool link(const std::filesystem::path exe);
};

void Linker::add_object(const std::filesystem::path& unit)
{
	files.push_back(unit);
}

bool Linker::link(const std::filesystem::path exe)
{
	using namespace std::filesystem;

	create_directories(exe.parent_path());

	auto compilerPath = ctx->gctx->clang_path();
	ProcessBuilder pb {compilerPath};

	for (auto& file : files)
	{
		pb.add_arg(file);
	}

	pb.add_arg("-o");
	pb.add_arg(exe);

	int result = pb.start();
	if (result != 0)
	{
		return false;
	}

	return true;
}

static char optlevel_to_char(OptLevel level)
{
	if (level <= OptLevel::LEVEL_3)
	{
		return static_cast<char>('0' + static_cast<int>(level));
	}
	else if (level == OptLevel::LEVEL_S)
	{
		return 's';
	}
	else if (level == OptLevel::LEVEL_Z)
	{
		return 'z';
	}

	std::unreachable();
}

static int debuglevel_to_int(DebugInfo level)
{
	return static_cast<int>(level);
}

static std::string standard_to_str(Standard standard)
{
	switch (standard)
	{
	case Standard::CXX23:
		return "c++23";
	}
}

using CompileUnitResult = std::optional<std::filesystem::path>;

CompileUnitResult static compile_unit(const Build& ctx,
	const Unit& unit,
	const CompileOptions& opts)
{
	using namespace std::filesystem;

	ProcessBuilder clangBase {ctx.gctx->clang_path()};

	clangBase.add_arg("-c");

	if (opts.debugLevel != DebugInfo::LEVEL_0)
	{
		clangBase.add_arg(std::format("-g{}", debuglevel_to_int(opts.debugLevel)));
	}

	if (opts.optLevel != OptLevel::LEVEL_0)
	{
		clangBase.add_arg(std::format("-O{}", optlevel_to_char(opts.optLevel)));
	}

	clangBase.add_arg(std::format("-std={}", standard_to_str(opts.standard)));

	std::vector<io::AnonymousFile> objectFiles;

	bool hadError = false;
	for (auto& sourceFile : expand_linear_paths(unit.target->paths))
	{
		ProcessBuilder clang {clangBase};
		clang.add_arg(sourceFile);

		auto objectFile = io::AnonymousFile::create();
		if (!objectFile.is_open())
		{
			bail("failed to build package");
		}

		clang.add_arg("-o");
		clang.add_arg(objectFile.path());

		int result = clang.start();
		if (result != 0)
		{
			hadError = true;
		}
		else
		{
			objectFiles.emplace_back(std::move(objectFile));
		}
	}

	if (hadError)
	{
		std::string binDescription =
			ctx.roots.size() > 1 ? std::format("(bin \"{}\")", unit.target->name) : "";
		print_error("could not compile `{}` {} due to error(s)",
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
		print_error("could not compile `{}` (bin \"{}\") due to linker error(s)",
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

static CompileResult compile(const Build& ctx, const CompileOptions& opts)
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

template<class R, class P> static float to_milliseconds(std::chrono::duration<R, P> d)
{
	using std::chrono::duration_cast;
	using std::chrono::milliseconds;
    constexpr static const double MILLISECONDS_PER_SECOND = 1000;
	return static_cast<double>(duration_cast<milliseconds>(d).count()) / MILLISECONDS_PER_SECOND;
}

static CompileResult build_package(const Workspace& ws,
	const Package& package,
	std::vector<std::string> targetsToBuild = {})
{
	using std::chrono::steady_clock;

	print_status("Compiling", "{} ({})", package.name(), package.root().string());

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
		opts.optLevel == OptLevel::LEVEL_0 ? "unoptimized" : "optimized";

	if (opts.debugLevel != DebugInfo::LEVEL_0)
	{
		description += " + debuginfo";
	}

	print_status(" Finished",
		"`{}` profile [{}] target(s) in {:.3}s",
		profile.name,
		description,
		to_milliseconds(timePassed));

	return result;
}

// static std::filesystem::path build_packages(const Workspace& ws) {
//     using std::chrono::steady_clock;

//     for (auto& package : ws.packages()) {
//         build_package(ws, package);
//     }

//     return binary;
// }

void exec_build(const BuildOptions&)
{
	using namespace std::filesystem;

	auto cwd = current_path();
	GlobalContext gctx {cwd};
	Workspace ws {cwd / "Freight.toml", gctx};
	build_package(ws, ws.current());
}

void exec_run(const RunOptions&)
{
	using namespace std::filesystem;

	auto cwd = current_path();
	GlobalContext gctx {cwd};
	Workspace ws {cwd / "Freight.toml", gctx};

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
	print_status("  Running", "`{}`", exePathRelative.string());

	ProcessBuilder pb {exePath};
	exit(pb.start());
}