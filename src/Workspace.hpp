#pragma once

#include <filesystem>
#include <ranges>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Toml.hpp>
#include <Support/Ranges.hpp>

namespace freight
{
enum class OptLevel
{
	Level0 = 0, // Don't optimize
	Level1,
	Level2,
	Level3,
	LevelS, // Optimize for size and speed
	LevelZ, // Optimize for size, even at the expense of speed
};

enum class DebugInfo
{
	Level0 = 0,
	Level1,
	Level2,
	Level3,
};

enum class Standard
{
	Cpp23,
};

struct Profile
{
	std::string name;
	// The relative path of this profile's subdirectory in `target/`
	std::filesystem::path target_subdir = {};
	OptLevel opt_level;
	DebugInfo debug;
	// If false, defines the `NDEBUG` macro
	bool debug_assertions = true;
	bool incremental;

	[[nodiscard]]
	static auto dev() -> Profile
	{
		return {
			.name = "dev",
			.target_subdir = "debug",
			.opt_level = OptLevel::Level0,
			.debug = DebugInfo::Level3,
			.debug_assertions = true,
			.incremental = true,
		};
	}

	[[nodiscard]]
	static auto release() -> Profile
	{
		return {
			.name = "release",
			.target_subdir = "release",
			.opt_level = OptLevel::Level3,
			.debug = DebugInfo::Level0,
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
	[[nodiscard]]
	static auto create(TomlManifest&& toml,
		const std::string name,
		std::vector<Target>&& targets,
		Standard standard) -> Manifest
	{
		return Manifest {Inner {
			.toml_ = std::move(toml),
			.name_ = std::move(name),
			.targets_ = std::move(targets),
			.standard_ = standard,
		}};
	}

	[[nodiscard]]
	auto name() const -> const std::string&
	{
		return inner.name_;
	}

	[[nodiscard]]
	auto targets() const -> std::span<const Target>
	{
		return inner.targets_;
	}

	[[nodiscard]]
	auto toml() const -> const TomlManifest&
	{
		return inner.toml_;
	}

	[[nodiscard]]
	auto standard() const -> Standard
	{
		return inner.standard_;
	}
private:
	struct Inner
	{
		TomlManifest toml_;
		std::string name_;
		std::vector<Target> targets_;
		Standard standard_;
	};

	Inner inner;

	explicit Manifest(Inner inner) : inner {std::move(inner)} {};
};

class Package
{
public:
	[[nodiscard]]
	static auto create(Manifest&& manifest, std::filesystem::path&& manifest_path)
		-> Package
	{
		return Package {std::move(manifest), std::move(manifest_path)};
	}

	[[nodiscard]]
	auto manifest_path() const -> const std::filesystem::path&
	{
		return manifest_path_;
	}

	[[nodiscard]]
	auto root() const -> std::filesystem::path
	{
		return manifest_path().parent_path();
	}

	[[nodiscard]]
	auto manifest() const -> const Manifest&
	{
		return manifest_;
	}

	[[nodiscard]]
	auto name() const -> const std::string&
	{
		return manifest().name();
	}

	[[nodiscard]]
	auto targets() const -> std::span<const Target>
	{
		return manifest().targets();
	}

	[[nodiscard]]
	auto standard() const -> Standard
	{
		return manifest().standard();
	}
private:
	Manifest manifest_;
	std::filesystem::path manifest_path_;

	explicit Package(Manifest&& manifest, std::filesystem::path manifest_path)
		: manifest_(std::move(manifest))
		, manifest_path_ {std::move(manifest_path)}
	{}
};

class GlobalContext
{
public:
	[[nodiscard]]
	static auto create(std::filesystem::path&& cwd) -> GlobalContext
	{
		return GlobalContext {std::move(cwd)};
	}

	[[nodiscard]]
	auto cwd() const -> const std::filesystem::path&
	{
		return cwd_;
	}

	[[nodiscard]]
	auto compiler_path() const -> const std::filesystem::path&;
private:
	std::filesystem::path cwd_;
	std::filesystem::path compiler_path_;

	explicit GlobalContext(std::filesystem::path&& cwd) : cwd_ {std::move(cwd)}
	{}
};

class Packages
{
public:
	[[nodiscard]]
	auto get(const std::filesystem::path& path) const -> const Package&
	{
		return packages_.at(path);
	}

	[[nodiscard]]
	auto get(const std::filesystem::path& path) -> Package&
	{
		return packages_.at(path);
	}

	template<class... Args>
	auto emplace(const std::filesystem::path& path, Args... args) -> Package&
	{
		return packages_.try_emplace(path, std::forward<Args>(args)...).first->second;
	}

	[[nodiscard]]
	auto view() const -> support::ranges::concepts::ViewOf<Package> auto
	{
		return std::views::values(packages_);
	}
private:
	std::unordered_map<std::filesystem::path, Package> packages_;
};

class Workspace
{
public:
	[[nodiscard]]
	static auto open(const std::filesystem::path& current_manifest, GlobalContext& gctx)
		-> Workspace
	{
		return Workspace {current_manifest, gctx};
	}

	[[nodiscard]]
	auto gctx() const -> const GlobalContext&
	{
		return *gctx_;
	}

	[[nodiscard]]
	auto root() const -> std::filesystem::path
	{
		return root_manifest().parent_path();
	}

	[[nodiscard]]
	auto root_manifest() const -> std::filesystem::path
	{
		return root_manifest_.value_or(current_manifest_);
	}

	[[nodiscard]]
	auto target_dir() const -> std::filesystem::path
	{
		return target_dir_.value_or(root() / "target");
	}

	[[nodiscard]]
	auto build_dir() const -> std::filesystem::path
	{
		return build_dir_.value_or(target_dir());
	}

	[[nodiscard]]
	auto members() const -> support::ranges::concepts::ViewOf<Package> auto
	{
		return packages.view();
	}

	[[nodiscard]]
	auto current() const -> const Package&
	{
		return packages.get(current_manifest_);
	}
private:
	GlobalContext *gctx_;
	std::filesystem::path current_manifest_;
	std::optional<std::filesystem::path> root_manifest_;
	std::optional<std::filesystem::path> target_dir_;
	std::optional<std::filesystem::path> build_dir_;
	Packages packages;

	explicit Workspace(const std::filesystem::path& current_manifest,
		GlobalContext& gctx);
};
} // namespace freight