#include "Pch.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <sys/mman.h>
#include <vector>

#include "Cmds.h"
#include "Io.h"
#include "Util.h"
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

struct Build
{
	const GlobalContext *gctx;
	const Workspace *workspace;
	std::vector<Unit> roots;
};

class Linker
{
private:
	const Build *_ctx;
	std::vector<std::filesystem::path> _files;
public:
	Linker(const Build& ctx) : _ctx {&ctx}
	{
	}

	void add_object(const std::filesystem::path& unit);
	bool link(const std::filesystem::path exe);
};

void Linker::add_object(const std::filesystem::path& unit)
{
	_files.push_back(unit);
}

bool Linker::link(const std::filesystem::path exe)
{
	using namespace std::filesystem;

	create_directories(exe.parent_path());

	auto compiler_path = _ctx->gctx->clang_path();
	ProcessBuilder pb {compiler_path};

	for (auto& file : _files)
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

	ProcessBuilder clang_base {ctx.gctx->clang_path()};

	clang_base.add_arg("-c");

	if (opts.debug_level != DebugInfo::LEVEL_0)
	{
		clang_base.add_arg(std::format("-g{}", debuglevel_to_int(opts.debug_level)));
	}

	if (opts.opt_level != OptLevel::LEVEL_0)
	{
		clang_base.add_arg(std::format("-O{}", optlevel_to_char(opts.opt_level)));
	}

	clang_base.add_arg(std::format("-std={}", standard_to_str(opts.standard)));

	std::vector<io::AnonymousFile> object_files;

	bool had_error = false;
	for (auto& source_file : expand_linear_paths(unit.target->paths))
	{
		ProcessBuilder clang {clang_base};
		clang.add_arg(source_file);

		auto object_file = io::AnonymousFile::create();
		if (!object_file.is_open())
		{
			bail("failed to build package");
		}

		clang.add_arg("-o");
		clang.add_arg(object_file.path());

		int result = clang.start();
		if (result != 0)
		{
			had_error = true;
		}
		else
		{
			object_files.emplace_back(std::move(object_file));
		}
	}

	if (had_error)
	{
		std::string bin_description =
			ctx.roots.size() > 1 ? std::format("(bin \"{}\")", unit.target->name) : "";
		print_error("could not compile `{}` {} due to error(s)",
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
		print_error("could not compile `{}` (bin \"{}\") due to linker error(s)",
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
	std::vector<std::string> targets_to_build = {})
{
	using std::chrono::steady_clock;

	status("Compiling", "{} ({})", package.name(), package.root().string());

	auto start_time = steady_clock::now();

	Build bctx {.gctx = &ws.gctx(), .workspace = &ws, .roots = {}};

	Profile profile = Profile::DEV_PROFILE;

	for (auto& target : package.targets())
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
		opts.opt_level == OptLevel::LEVEL_0 ? "unoptimized" : "optimized";

	if (opts.debug_level != DebugInfo::LEVEL_0)
	{
		description += " + debuginfo";
	}

	status(" Finished",
		"`{}` profile [{}] target(s) in {:.3}s",
		profile.name,
		description,
		to_milliseconds(time_passed));

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
	Workspace ws {cwd / "Stim.toml", gctx};
	build_package(ws, ws.current());
}

void exec_run(const RunOptions&)
{
	using namespace std::filesystem;

	auto cwd = current_path();
	GlobalContext gctx {cwd};
	Workspace ws {cwd / "Stim.toml", gctx};

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

	auto exe_path = result.binaries.front();
	auto exe_path_relative = relative(result.binaries.front(), gctx.cwd());
	status("  Running", "`{}`", exe_path_relative.string());

	ProcessBuilder pb {exe_path};
	exit(pb.start());
}