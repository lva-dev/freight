#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

struct TomlPackage
{
	std::optional<std::string> name;
    std::optional<std::string> version;
	std::optional<std::string> standard;
};

struct TomlTarget
{
	std::optional<std::string> name;
	std::optional<std::vector<std::filesystem::path>> paths;
};

struct TomlManifest
{
	std::optional<TomlPackage> package;
	std::optional<std::vector<TomlTarget>> bin;
};

TomlManifest serialize_toml(const std::filesystem::path& manifest_path);