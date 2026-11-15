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

static auto collect_target_sources(const std::filesystem::path& dir)
	-> CollectResult<std::filesystem::path>
{
	using namespace std::filesystem;

	std::vector<path> paths;
	for (const auto& entry : directory_iterator {dir})
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

static auto infer_binary_targets(const std::filesystem::path& dir)
	-> CollectResult<Target>
{
	using namespace std::filesystem;

	std::vector<Target> targets;

	for (const auto& entry : directory_iterator {dir})
	{
		if (is_real_cpp_file(entry))
		{
			const auto& file = entry.path();
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

static auto infer_targets(GlobalContext& gctx, const std::string& package_name)
	-> CollectResult<Target>
{
	using namespace std::filesystem;

	std::vector<Target> targets;
	std::vector<path> main_target_paths;

	for (const auto& entry : directory_iterator(gctx.cwd() / "src"))
	{
		if (is_real_cpp_file(entry))
		{
			main_target_paths.push_back(entry);
		}
		else if (entry.is_directory())
		{
			if (entry.path().filename() == "bin")
			{
				auto binary_targets = infer_binary_targets(entry);
				if (!binary_targets)
				{
					return std::unexpected(binary_targets.error());
				}

				support::ranges::move_back_range(targets, *binary_targets);
			}
			else
			{
				main_target_paths.push_back(entry);
			}
		}
	}

	if (!main_target_paths.empty())
	{
		targets.emplace_back(package_name, main_target_paths);
	}

	return targets;
}

class ManifestReaderState
{
public:
	ManifestReaderState(const std::filesystem::path manifest_path, GlobalContext& gctx)
		: gctx_ {&gctx}
		, manifest_path_ {manifest_path}
	{}

	auto gctx() const -> GlobalContext&
	{
		return *gctx_;
	}

	auto manifest_path() const -> const std::filesystem::path&
	{
		return manifest_path_;
	}

	[[noreturn]]
	void fail(std::string_view message) const
	{
		error::bail(
			"failed to parse manifest at `{}`\n\n{}", manifest_path().native(), message);
	}
private:
	GlobalContext *gctx_;
	std::filesystem::path manifest_path_;
};

static auto infer_target_path(ManifestReaderState& mrs, const std::string target_name)
	-> std::filesystem::path
{
	using namespace std::filesystem;

	auto bin = mrs.manifest_path().parent_path() / "src/bin";

	auto source_file1 = (bin / target_name).replace_extension(".cpp");
	bool found_source1 = is_regular_file(source_file1);

	auto source_file2 = bin / target_name / "main.cpp";
	bool found_source2 = is_regular_file(source_file2);

	if (found_source1 && found_source2)
	{
		mrs.fail(
			error::cause("cannot infer path for `{0}` bin\n"
						 "Cargo doesn't know which to use because multiple target files"
						 " found at `src/bin/{0}/main.c` and `src/bin/{0}.c`.",
				target_name));
	}

	if (found_source1)
	{
		return source_file1;
	}
	
    if (found_source2)
	{
		return source_file2;
	}

	mrs.fail(
		error::cause("can't find `{0}` bin at `src/bin/{0}.c` or `src/bin/{0}/main.c`.\n"
					 "Please specify bin.path if you want to use a non-default path.",
			target_name));
}

static auto parse_standard(std::string_view standard) -> std::optional<Standard>
{
	if (standard == "23")
	{
		return Standard::Cpp23;
	}
    // TODO: Add more standards
    
    return {};
}

static auto read_manifest(GlobalContext& gctx, const std::filesystem::path& path)
	-> Manifest
{
	TomlManifest toml_manifest = serialize_toml(path);

	ManifestReaderState mrs {path, gctx};

	// TODO: Check deserialized manifest for package name instead
	std::string package_name = path.parent_path().filename();

	std::vector<Target> targets;
	if (toml_manifest.bin)
	{
		// TODO: Define structs for `toml_manifest` and `toml_target`
		auto& toml_bin_targets = *toml_manifest.bin;
		for (auto& toml_target : toml_bin_targets)
		{
			Target target {
				.name = *toml_target.name,
				.paths {},
			};

			if (toml_target.paths)
			{
				for (auto& path : *toml_target.paths)
				{
					if (!exists(path))
					{
						// TODO: Implement logic for when a path specified for a target
						// doesn't exist
					}
				}

				target.paths = *toml_target.paths;
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
		auto inferred_targets = infer_targets(gctx, package_name);
		if (!inferred_targets)
		{
			error::bail("source file `{}` is not a regular file",
				inferred_targets.error().path().native());
		}

		support::ranges::move_back_range(targets, *inferred_targets);
	}

	if (targets.empty())
	{
		mrs.fail(
			error::cause("no targets specified in the manifest\n"
						 "  either src/lib.rs, src/main.rs, a [lib] section, or"
						 " [[bin]] section must be present"));
	}

	Standard standard {};
	if (toml_manifest.package && toml_manifest.package->standard)
	{
		auto& value = *toml_manifest.package->standard;
		auto stdopt = parse_standard(value);
		if (!stdopt)
		{
			// TODO: Emit error or error::bail
			mrs.fail(std::format("{}\n\n{}",
				error::cause("failed to parse the `standard` key"),
				error::cause(std::format(
					"supported standard values are `23`, but `{}` is unknown", value))));
		}

        standard = *stdopt;
	}
	else
	{
		standard = Standard::Cpp23;
	}

	return Manifest::create(
		std::move(toml_manifest), std::move(package_name), std::move(targets), standard);
}

static auto find_root_manifest(const std::filesystem::path& current_manifest)
	-> std::optional<std::filesystem::path>
{
	std::filesystem::path dir;
	for (const auto& component : current_manifest.parent_path())
	{
		dir /= component;
		auto expected_manifest = dir / "Freight.toml";
		if (exists(expected_manifest))
		{
			return expected_manifest;
		}
	}

	return {};
}

freight::Workspace::Workspace(const std::filesystem::path& current_manifest,
	GlobalContext& gctx)
	: gctx_ {&gctx}
{

	if (!std::filesystem::exists(current_manifest))
	{
		root_manifest_ = find_root_manifest(current_manifest);
		if (!root_manifest_)
		{
			error::bail("could not find `Freight.toml` in `{}` or any parent directory",
				gctx.cwd().native());
		}

		current_manifest_ = *root_manifest_;
	}
	else
	{
		current_manifest_ = current_manifest;
	}

	auto inferred_package = Package::create(
		read_manifest(gctx, current_manifest), std::filesystem::path {current_manifest});
	packages.emplace(current_manifest, std::move(inferred_package));
}

static auto search_path(const std::filesystem::path& file)
	-> std::optional<std::filesystem::path>
{
	using namespace std::filesystem;

	gsl::zstring path_env = std::getenv("PATH");
	if (path_env == nullptr)
	{
		return {};
	}

	gsl::zstring dir_token = std::strtok(path_env, ":");
	while (dir_token != nullptr)
	{
		auto joined = path(dir_token) / file;
		if (exists(joined))
		{
			return joined;
		}

		dir_token = std::strtok(nullptr, ":");
	}

	return {};
}

auto freight::GlobalContext::compiler_path() const -> const std::filesystem::path&
{
	static std::filesystem::path CACHED_PATH;

	if (!compiler_path_.empty())
	{
		CACHED_PATH.clear();
		return compiler_path_;
	}

	if (CACHED_PATH.empty())
	{
		auto path = search_path("clang++");
		if (!path.has_value())
		{
			// TODO: Error if clang++ isn't installed
			assert(false && "Failed to find clang++");
		}

		CACHED_PATH = *std::move(path);
	}

	return CACHED_PATH;
}
