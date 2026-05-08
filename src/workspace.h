#pragma once

#include <filesystem>
#include <ranges>
#include <unordered_map>
#include <vector>

#include "toml.h"

enum class OptLevel {
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

enum class DebugInfo {
	LEVEL_0 = 0,
	LEVEL_1,
	LEVEL_2,
	LEVEL_3,
};

enum class Standard {
	CXX23,
};

struct Profile {
	std::string name;
	// The relative path of this profile's subdirectory in `target/`
	std::filesystem::path target_subdir = "";
	OptLevel opt_level;
	DebugInfo debug;
	// If false, defines the `NDEBUG` macro
	bool debug_assertions = true;
	bool incremental;

	static const Profile DEV_PROFILE;
	static const Profile RELEASE_PROFILE;
};

inline const Profile Profile::DEV_PROFILE = {
	.name = "dev",
	.target_subdir = "debug",
	.opt_level = OptLevel::LEVEL_0,
	.debug = DebugInfo::LEVEL_3,
	.debug_assertions = true,
	.incremental = true,
};

inline const Profile Profile::RELEASE_PROFILE = {
	.name = "release",
	.target_subdir = "release",
	.opt_level = OptLevel::LEVEL_3,
	.debug = DebugInfo::LEVEL_0,
	.debug_assertions = false,
	.incremental = false,
};

struct Target {
	std::string name;
	std::vector<std::filesystem::path> paths;
};

class Manifest {
	TomlManifest _toml;
	std::string _name;
	std::vector<Target> _targets;
	Standard _standard;
public:
	Manifest(TomlManifest&& toml,
		const std::string& name,
		std::vector<Target>&& targets,
		Standard standard)
		: _toml {std::move(toml)},
		  _name {name},
		  _targets {std::move(targets)},
		  _standard {standard} {
	}

	const std::string& name() const {
		return _name;
	}

	std::span<const Target> targets() const {
		return _targets;
	}

	const TomlManifest& toml() {
		return _toml;
	}

	Standard standard() const {
		return _standard;
	}
};

class Package {
private:
	Manifest _manifest;
	std::filesystem::path _manifest_path;
public:
	Package(Manifest&& manifest, const std::filesystem::path& _manifest_path)
		: _manifest(manifest),
		  _manifest_path {_manifest_path} {
	}

	const std::filesystem::path& manifest_path() const {
		return _manifest_path;
	}

	std::filesystem::path root() const {
		return manifest_path().parent_path();
	}

	const Manifest& manifest() const {
		return _manifest;
	}

	const std::string& name() const {
		return manifest().name();
	}

	std::span<const Target> targets() const {
		return manifest().targets();
	}

	Standard standard() const {
		return manifest().standard();
	}
};

class GlobalContext {
private:
	std::filesystem::path _cwd;
	std::filesystem::path _compiler_path;
public:
	GlobalContext(const std::filesystem::path& cwd) : _cwd {cwd} {
	}

	const std::filesystem::path& cwd() const {
		return _cwd;
	}

	const std::filesystem::path& clang_path() const;
};

class Packages {
private:
	std::unordered_map<std::filesystem::path, Package> _packages;
public:
	using Container = decltype(_packages);

	Packages() = default;
	Packages(const Packages&) = default;
	Packages& operator=(const Packages&) = default;
	Packages(Packages&&) = default;
	Packages& operator=(Packages&&) = default;

	const Package& operator[](const std::filesystem::path& path) const {
		return _packages.at(path);
	}

	Package& operator[](const std::filesystem::path& path) {
		return _packages.at(path);
	}

	Package& insert(const std::filesystem::path& path, const Package& package) {
		return _packages.insert({path, package}).first->second;
	}

	template<class... Args>
	Package& emplace(const std::filesystem::path& path, Args... args) {
		return _packages.try_emplace(path, std::forward<Args>(args)...).first->second;
	}

	const Container& container() const {
		return _packages;
	}
};

class Workspace {
private:
	const GlobalContext& _gctx;
	std::filesystem::path _current_manifest;
	std::optional<std::filesystem::path> _root_manifest;
	std::optional<std::filesystem::path> _target_dir;
	std::optional<std::filesystem::path> _object_dir;
	Packages _packages;

	Workspace(const GlobalContext& gctx,
		std::filesystem::path&& current_manifest,
		std::optional<std::filesystem::path>&& root_manifest,
		std::filesystem::path&& target_dir,
		std::filesystem::path&& object_dir,
		Packages&& packages)
		: _gctx {gctx},
		  _current_manifest {current_manifest},
		  _root_manifest {root_manifest},
		  _target_dir {target_dir},
		  _object_dir {object_dir},
		  _packages {packages} {
	}
public:
	Workspace(const std::filesystem::path& current_manifest, const GlobalContext& gctx);

	static Workspace open(const GlobalContext& gctx);

	const GlobalContext& gctx() const {
		return _gctx;
	}

	std::filesystem::path root() const {
		return root_manifest().parent_path();
	}

	std::filesystem::path root_manifest() const {
		return _root_manifest.value_or(_current_manifest);
	}

	std::filesystem::path target_dir() const {
		return _target_dir.value_or(root() / "target");
	}

	std::filesystem::path build_dir() const {
		return _object_dir.value_or(target_dir());
	}

	auto members() const {
		return std::views::values(_packages.container());
	}

	const Package& current() const {
		return _packages[_current_manifest];
	}
};