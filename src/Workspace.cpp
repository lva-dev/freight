#include "Pch.h"

#include "Workspace.h"

#include <cstring>
#include <expected>
#include <filesystem>
#include <gsl/zstring>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

#include "Toml.h"
#include "Support/Util.h"

static bool is_real_cpp_file(const std::filesystem::path file)
{
	return std::filesystem::is_regular_file(file) && file.extension() == ".cpp";
}

template<class T>
using CollectResult = std::expected<std::vector<T>, std::filesystem::directory_entry>;

static CollectResult<std::filesystem::path> collect_target_sources(
	const std::filesystem::path& dir)
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

static CollectResult<Target> infer_binary_targets(const std::filesystem::path& dir)
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

static CollectResult<Target> infer_targets(GlobalContext& gctx,
	const std::string& packageName)
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

				ranges::move_back_range(targets, *binaryTargets);
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
		: gctx_ {&gctx},
		  manifestPath {manifestPath}
	{
	}

	GlobalContext& gctx() const
	{
		return *gctx_;
	}

	const std::filesystem::path& manifest_path() const
	{
		return manifestPath;
	}

	[[noreturn]] void fail(std::string_view message) const
	{
		bail("failed to parse manifest at `{}`\n\n{}", manifestPath.string(), message);
	}
};

static std::filesystem::path infer_target_path(ManifestReaderState& mrs,
	const std::string targetName)
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
			cause("cannot infer path for `{0}` bin\n"
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
		cause("can't find `{0}` bin at `src/bin/{0}.c` or `src/bin/{0}/main.c`.\n"
			  "Please specify bin.path if you want to use a non-default path.",
			targetName));
}

static std::optional<Standard> parse_standard(std::string_view standard)
{
	if (standard == "23")
	{
		return Standard::CXX23;
	}
	else
	{
		return {};
	}
}

static Manifest read_manifest(GlobalContext& gctx,
	const std::filesystem::path& manifestPath)
{
	TomlManifest tomlManifest = serialize_toml(manifestPath);

	ManifestReaderState mrs {manifestPath, gctx};

	// TODO: Check deserialized manifest for package name instead
	std::string packageName = manifestPath.parent_path().filename();

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
			bail("source file `{}` is not a regular file",
				inferredTargets.error().path().string());
		}

		ranges::move_back_range(targets, *inferredTargets);
	}

	if (targets.empty())
	{
		mrs.fail(
			cause("no targets specified in the manifest\n"
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
			// TODO: Emit error or bail
			mrs.fail(std::format("{}\n\n{}",
				cause("failed to parse the `standard` key"),
				cause(std::format(
					"supported standard values are `23`, but `{}` is unknown", value))));
		}
		else
		{
			standard = *stdopt;
		}
	}
	else
	{
		standard = Standard::CXX23;
	}

	return Manifest {std::move(tomlManifest), packageName, std::move(targets), standard};
}

static std::optional<std::filesystem::path> find_manifest(
	const std::filesystem::path& manifestPath)
{
	std::filesystem::path dir;
	for (auto& component : manifestPath.parent_path())
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

Workspace::Workspace(const std::filesystem::path& currentManifest, GlobalContext& gctx)
	: gctx_ {&gctx}
{

	rootManifest = find_manifest(currentManifest);

	if (!std::filesystem::exists(currentManifest))
	{
		if (!rootManifest)
		{
			bail("could not find `Freight.toml` in `{}` or any parent directory",
				gctx.cwd().string());
		}
		else
		{
			currentManifest_ = *rootManifest;
		}
	}
	else
	{
		currentManifest_ = currentManifest;
	}

	packages.emplace(currentManifest,
		Package {
			read_manifest(gctx, currentManifest),
			currentManifest,
		});
}

static std::filesystem::path search_path(const std::filesystem::path& file)
{
	using namespace std::filesystem;

	gsl::zstring pathEnv = getenv("PATH");
	if (!pathEnv)
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

const std::filesystem::path& GlobalContext::clang_path() const
{
	static std::filesystem::path cachedPath;

	if (!compilerPath.empty())
	{
		cachedPath.clear();
		return compilerPath;
	}

	if (cachedPath.empty())
	{
		cachedPath = search_path("clang++");
	}

	return cachedPath;
}