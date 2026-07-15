#pragma once

#include <filesystem>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "Toml.h"

enum class OptLevel
{
	// Don't optimize
	LEVEL_0 = 0,
	LEVEL_1,
	LEVEL_2,
	LEVEL_3,
	// Optimize for size and speed
	LEVEL_S,
	// Optimize for size, even at the expense of speed
	LEVEL_Z,
};

enum class DebugInfo
{
	LEVEL_0 = 0,
	LEVEL_1,
	LEVEL_2,
	LEVEL_3,
};

enum class Standard
{
	CXX23,
};

struct Profile
{
	std::string name;
	// The relative path of this profile's subdirectory in `target/`
	std::filesystem::path target_subdir = "";
	OptLevel optLevel;
	DebugInfo debug;
	// If false, defines the `NDEBUG` macro
	bool debug_assertions = true;
	bool incremental;

	static Profile dev()
	{
		return {
			.name = "dev",
			.target_subdir = "debug",
			.optLevel = OptLevel::LEVEL_0,
			.debug = DebugInfo::LEVEL_3,
			.debug_assertions = true,
			.incremental = true,
		};
	}

	static Profile release()
	{
		return {
			.name = "release",
			.target_subdir = "release",
			.optLevel = OptLevel::LEVEL_3,
			.debug = DebugInfo::LEVEL_0,
			.debug_assertions = false,
			.incremental = false,
		};
	}
};

struct Target
{
	std::string name;
	std::vector<std::filesystem::path> paths;
};

class Manifest
{
public:
	Manifest(TomlManifest&& toml,
		const std::string& name,
		std::vector<Target>&& targets,
		Standard standard)
		: toml_ {std::move(toml)},
		  name_ {name},
		  targets_ {std::move(targets)},
		  standard_ {standard}
	{
	}

	const std::string& name() const
	{
		return name_;
	}

	std::span<const Target> targets() const
	{
		return targets_;
	}

	const TomlManifest& toml()
	{
		return toml_;
	}

	Standard standard() const
	{
		return standard_;
	}
private:
	TomlManifest toml_;
	std::string name_;
	std::vector<Target> targets_;
	Standard standard_;
};

class Package
{
public:
	Package(Manifest&& manifest, std::filesystem::path manifestPath)
		: manifest_(std::move(manifest)),
		  manifestPath {std::move(manifestPath)}
	{
	}

	const std::filesystem::path& manifest_path() const
	{
		return manifestPath;
	}

	std::filesystem::path root() const
	{
		return manifest_path().parent_path();
	}

	const Manifest& manifest() const
	{
		return manifest_;
	}

	const std::string& name() const
	{
		return manifest().name();
	}

	std::span<const Target> targets() const
	{
		return manifest().targets();
	}

	Standard standard() const
	{
		return manifest().standard();
	}
private:
	Manifest manifest_;
	std::filesystem::path manifestPath;
};

class GlobalContext
{
private:
	std::filesystem::path cwd_;
	std::filesystem::path compilerPath;
public:
	GlobalContext(std::filesystem::path cwd) : cwd_ {std::move(cwd)}
	{
	}

	const std::filesystem::path& cwd() const
	{
		return cwd_;
	}

	const std::filesystem::path& clang_path() const;
};

class Packages
{
private:
	std::unordered_map<std::filesystem::path, Package> packages_;
public:
	using Container = decltype(packages_);

	Packages() = default;
	~Packages() = default;
	Packages(const Packages&) = default;
	Packages& operator=(const Packages&) = default;
	Packages(Packages&&) = default;
	Packages& operator=(Packages&&) = default;

	const Package& operator[](const std::filesystem::path& path) const
	{
		return packages_.at(path);
	}

	Package& operator[](const std::filesystem::path& path)
	{
		return packages_.at(path);
	}

	Package& insert(const std::filesystem::path& path, const Package& package)
	{
		return packages_.insert({path, package}).first->second;
	}

	template<class... Args>
	Package& emplace(const std::filesystem::path& path, Args... args)
	{
		return packages_.try_emplace(path, std::forward<Args>(args)...).first->second;
	}

	const Container& container() const
	{
		return packages_;
	}
};

class Workspace
{
private:
	GlobalContext *gctx_;
	std::filesystem::path currentManifest_;
	std::optional<std::filesystem::path> rootManifest;
	std::optional<std::filesystem::path> targetDir;
	std::optional<std::filesystem::path> objectDir;
	Packages packages;

	Workspace(GlobalContext& gctx,
		std::filesystem::path&& current_manifest,
		std::optional<std::filesystem::path>&& root_manifest,
		std::optional<std::filesystem::path>&& target_dir,
		std::optional<std::filesystem::path>&& object_dir,
		Packages&& packages)
		: gctx_ {&gctx},
		  currentManifest_ {std::move(current_manifest)},
		  rootManifest {std::move(root_manifest)},
		  targetDir {std::move(target_dir)},
		  objectDir {std::move(object_dir)},
		  packages {std::move(packages)}
	{
	}
public:
	Workspace(const std::filesystem::path& current_manifest, GlobalContext& gctx);

	static Workspace open(const GlobalContext& gctx);

	const GlobalContext& gctx() const
	{
		return *gctx_;
	}

	std::filesystem::path root() const
	{
		return root_manifest().parent_path();
	}

	std::filesystem::path root_manifest() const
	{
		return rootManifest.value_or(currentManifest_);
	}

	std::filesystem::path target_dir() const
	{
		return targetDir.value_or(root() / "target");
	}

	std::filesystem::path build_dir() const
	{
		return objectDir.value_or(target_dir());
	}

	auto members() const
	{
		return std::views::values(packages.container());
	}

	const Package& current() const
	{
		return packages[currentManifest_];
	}
};