#include "Pch.hpp"

#include "tomlplusplus/tomlplusplus.h"

#include "Error.hpp"
#include "Workspace.hpp"

using namespace freight;

void print_parse_error(const toml::parse_error& error)
{
	error::print_error("{}", error.description());
	auto src = error.source();
	std::println(std::cerr, " --> {}:{}:{}", *src.path, src.end.line, src.end.column);
}

auto serialize_toml([[maybe_unused]] const std::filesystem::path& manifestPath) -> TomlManifest
{
	toml::parse_result result = toml::parse_file(manifestPath.string());
	if (!result)
	{
		// TODO: Return error
	}

	auto table = result.table();
	TomlManifest manifest;

	auto package = table["package"];
	if (package.is_table())
	{
		manifest.package = TomlPackage {};

		if (package["name"].is_string())
		{
			manifest.package->name = package["name"].as_string()->get();
		}
        else
        {
            // TODO: Return error
        }

		if (package["version"].is_string())
		{
			manifest.package->version = package["version"].as_string()->get();
		}

		if (package["standard"].is_string())
		{
			manifest.package->standard = package["standard"].as_string()->get();
		}
	}

	return manifest;
}
