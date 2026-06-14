#include "Pch.h"
#include "tomlplusplus/tomlplusplus.h"
#include "Util.h"
#include "Workspace.h"

void print_parse_error(const toml::parse_error& error) {
    emit_error("{}", error.description());
    auto src = error.source();
    std::println(std::cerr, " --> {}:{}:{}", *src.path, src.end.line, src.end.column);
}

TomlManifest serialize_toml([[maybe_unused]] const std::filesystem::path& manifest_path)
{
    toml::parse_result result = toml::parse_file(manifest_path.string());
    if (!result) {
        // TODO: Emit error
        bail("");
    }
    
    auto table = result.table();
    TomlManifest manifest;

    auto package = table["package"];
    if (package.is_table()) {
        manifest.package = TomlPackage{};        
        
        if (package["name"].is_string()) {
            manifest.package->name = package["name"].as_string()->get();
        }

        if (package["version"].is_string()) {
            manifest.package->version = package["version"].as_string()->get();
        }

        if (package["standard"].is_string()) {
            manifest.package->standard = package["standard"].as_string()->get();
        }
    }
    
	return manifest;
}
