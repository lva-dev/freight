#include "Pch.hpp"

#include "Workspace.hpp"

#include <gsl/zstring>

#include <cstring>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "Error.hpp"
#include "Toml.hpp"
#include "Support/Ranges.hpp"

using namespace freight;

static auto is_real_cpp_file(const std::filesystem::path file) -> bool
{
	return std::filesystem::is_regular_file(file) && file.extension() == ".cpp";
}

template<class T>
using CollectResult = std::expected<std::vector<T>, std::filesystem::directory_entry>;

static auto collect_target_sources(
	const std::filesystem::path& dir) -> CollectResult<std::filesystem::path>
{
	using namespace std::filesystem;

	std::vector<path> paths;
	for (auto& entry : directory_iterator {dir})
	{
		if (is_real_cpp_file(entry) || entry.is_directory())
		{
			paths.push_back(entry.path());
		}
		else
		{
			return std::unexpected {entry};
		}
	}

	return paths;
}

static auto infer_binary_targets(const std::filesystem::path& dir) -> CollectResult<Target>
{
	using namespace std::filesystem;

	std::vector<Target> targets;

	for (auto& entry : directory_iterator {dir})
	{
		if (is_real_cpp_file(entry))
		{
			auto& file = entry.path();
			targets.emplace_back(file.stem(), std::vector {file});
		}
		else if (entry.is_directory())
		{
			auto paths = collect_target_sources(entry);
			if (!paths)
			{
				return std::unexpected {paths.error()};
			}

			targets.emplace_back(entry.path().filename(), std::move(*paths));
		}
	}

	return targets;
}

static auto infer_targets(GlobalContext& gctx,
	const std::string& packageName) -> CollectResult<Target>
{
	using namespace std::filesystem;

	std::vector<Target> targets;
	std::vector<path> mainTargetPaths;

	for (auto& entry : directory_iterator(gctx.cwd() / "src"))
	{
		if (is_real_cpp_file(entry))
		{
			mainTargetPaths.push_back(entry);
		}
		else if (entry.is_directory())
		{
			if (entry.path().filename() == "bin")
			{
				auto binaryTargets = infer_binary_targets(entry);
				if (!binaryTargets)
				{
					return std::unexpected(binaryTargets.error());
				}

				support::ranges::move_back_range(targets, *binaryTargets);
			}
			else
			{
				mainTargetPaths.push_back(entry);
			}
		}
	}

	if (!mainTargetPaths.empty())
	{
		targets.emplace_back(packageName, mainTargetPaths);
	}

	return targets;
}

class ManifestReaderState
{
	GlobalContext *gctx_;
	std::filesystem::path manifestPath;
public:
	ManifestReaderState(const std::filesystem::path manifestPath, GlobalContext& gctx)
		: gctx_ {&gctx}
		, manifestPath {manifestPath}
	{}

	auto gctx() const -> GlobalContext&
	{
		return *gctx_;
	}

	auto manifest_path() const -> const std::filesystem::path&
	{
		return manifestPath;
	}

	[[noreturn]] void fail(std::string_view message) const
	{
		error::bail(
			"failed to parse manifest at `{}`\n\n{}", manifestPath.native(), message);
	}
};

static auto infer_target_path(ManifestReaderState& mrs,
	const std::string targetName) -> std::filesystem::path
{
	using namespace std::filesystem;

	auto bin = mrs.manifest_path().parent_path() / "src/bin";

	auto sourceFile1 = (bin / targetName).replace_extension(".cpp");
	bool foundSource1 = is_regular_file(sourceFile1);

	auto sourceFile2 = bin / targetName / "main.cpp";
	bool foundSource2 = is_regular_file(sourceFile2);

	if (foundSource1 && foundSource2)
	{
		mrs.fail(
			error::cause("cannot infer path for `{0}` bin\n"
						 "Cargo doesn't know which to use because multiple target files"
						 " found at `src/bin/{0}/main.c` and `src/bin/{0}.c`.",
				targetName));
	}
	else if (foundSource1)
	{
		return sourceFile1;
	}
	else if (foundSource2)
	{
		return sourceFile2;
	}

	mrs.fail(
		error::cause("can't find `{0}` bin at `src/bin/{0}.c` or `src/bin/{0}/main.c`.\n"
					 "Please specify bin.path if you want to use a non-default path.",
			targetName));
}

static auto parse_standard(std::string_view standard) -> std::optional<Standard>
{
	if (standard == "23")
	{
		return Standard::Cpp23;
	}
	else
	{
		return {};
	}
}

static auto read_manifest(GlobalContext& gctx, const std::filesystem::path& path) -> Manifest
{
	TomlManifest tomlManifest = serialize_toml(path);

	ManifestReaderState mrs {path, gctx};

	// TODO: Check deserialized manifest for package name instead
	std::string packageName = path.parent_path().filename();

	std::vector<Target> targets;
	if (tomlManifest.bin)
	{
		// TODO: Define structs for `toml_manifest` and `toml_target`
		auto& tomlBinTargets = *tomlManifest.bin;
		for (auto& tomlTarget : tomlBinTargets)
		{
			Target target {
				.name = *tomlTarget.name,
				.paths {},
			};

			if (tomlTarget.paths)
			{
				for (auto& path : *tomlTarget.paths)
				{
					if (!exists(path))
					{
						// TODO: Implement logic for when a path specified for a target
						// doesn't exist
					}
				}

				target.paths = *tomlTarget.paths;
			}
			else
			{
				target.paths.push_back(infer_target_path(mrs, target.name));
			}

			targets.emplace_back(std::move(target));
		}
	}
	else
	{
		auto inferredTargets = infer_targets(gctx, packageName);
		if (!inferredTargets)
		{
			error::bail("source file `{}` is not a regular file",
				inferredTargets.error().path().native());
		}

		support::ranges::move_back_range(targets, *inferredTargets);
	}

	if (targets.empty())
	{
		mrs.fail(
			error::cause("no targets specified in the manifest\n"
						 "  either src/lib.rs, src/main.rs, a [lib] section, or"
						 " [[bin]] section must be present"));
	}

	Standard standard {};
	if (tomlManifest.package && tomlManifest.package->standard)
	{
		auto& value = *tomlManifest.package->standard;
		auto stdopt = parse_standard(value);
		if (!stdopt)
		{
			// TODO: Emit error or error::bail
			mrs.fail(std::format("{}\n\n{}",
				error::cause("failed to parse the `standard` key"),
				error::cause(std::format(
					"supported standard values are `23`, but `{}` is unknown", value))));
		}
		else
		{
			standard = *stdopt;
		}
	}
	else
	{
		standard = Standard::Cpp23;
	}

	return Manifest::create(
		std::move(tomlManifest), std::move(packageName), std::move(targets), standard);
}

static auto find_root_manifest(
	const std::filesystem::path& currentManifest) -> std::optional<std::filesystem::path>
{
	std::filesystem::path dir;
	for (auto& component : currentManifest.parent_path())
	{
		dir /= component;
		auto expectedManifest = dir / "Freight.toml";
		if (exists(expectedManifest))
		{
			return expectedManifest;
		}
	}

	return {};
}

freight::Workspace::Workspace(const std::filesystem::path& currentManifest,
	GlobalContext& gctx)
	: gctx_ {&gctx}
{
    
    if (!std::filesystem::exists(currentManifest))
	{
        rootManifest = find_root_manifest(currentManifest);
		if (!rootManifest)
		{
			error::bail("could not find `Freight.toml` in `{}` or any parent directory",
				gctx.cwd().native());
		}

        currentManifest_ = *rootManifest;
	}
	else
	{
		currentManifest_ = currentManifest;
	}

	auto inferredPackage = Package::create(
		read_manifest(gctx, currentManifest), std::filesystem::path {currentManifest});
	packages.emplace(currentManifest, std::move(inferredPackage));
}

static auto search_path(const std::filesystem::path& file) -> std::optional<std::filesystem::path>
{
	using namespace std::filesystem;

	gsl::zstring pathEnv = std::getenv("PATH");
	if (pathEnv == nullptr)
	{
		return {};
	}

	gsl::zstring dirToken = std::strtok(pathEnv, ":");
	while (dirToken)
	{
		auto joined = path(dirToken) / file;
		if (exists(joined))
		{
			return joined;
		}

		dirToken = std::strtok(nullptr, ":");
	}

	return {};
}

auto freight::GlobalContext::compiler_path() const -> const std::filesystem::path&
{
	static std::filesystem::path cachedPath;

	if (!compilerPath.empty())
	{
		cachedPath.clear();
		return compilerPath;
	}

	if (cachedPath.empty())
	{
        auto path = search_path("clang++");
        if (!path.has_value())
        {
            // TODO: Error is clang++ isn't installed
            assert(false && "Failed to find clang++");
        }
        
		cachedPath = *std::move(path);
	}

	return cachedPath;
}
