#include "workspace.h"
#include "toml.h"
#include "util.h"
#include <assert.h>
#include <expected>
#include <filesystem>
#include <optional>
#include <string>
#include <sys/stat.h>
#include <utility>
#include <vector>

char optlevel_to_char(OptLevel level) {
    if (level <= OptLevel::LEVEL_3) {
        return '0' + static_cast<int>(level);
    } else if (level == OptLevel::LEVEL_S) {
        return 's';
    } else if (level == OptLevel::LEVEL_Z) {
        return 'z';
    }

    std::unreachable();
}

template<class T>
using CollectResult = std::expected<std::vector<T>, std::filesystem::directory_entry>;

static CollectResult<std::filesystem::path>
    collect_target_sources(const std::filesystem::path& dir) {
    using namespace std::filesystem;
    
    std::vector<path> paths;
    for (auto& entry : directory_iterator{dir}) {
        if (entry.is_regular_file() || entry.is_directory()) {
            paths.push_back(entry.path());
        } else {
            return std::unexpected{entry};
        }            
    }

    return paths;    
}

static CollectResult<Target> infer_binary_targets(const std::filesystem::path& dir) {
    using namespace std::filesystem;

    std::vector<Target> targets;
    
    for (auto& entry : directory_iterator{dir}) {
        if (entry.is_regular_file()) {
            auto& file = entry.path();
            targets.emplace_back(file.stem(), std::vector{file});
        } else if (entry.is_directory()) {
            auto paths = collect_target_sources(entry);
            if (!paths) {
                return std::unexpected{paths.error()};
            }

            targets.emplace_back(entry.path().filename(), std::move(*paths));
        } else {
            return std::unexpected{entry};
        }
    }

    return targets;    
}

static CollectResult<Target> infer_targets(const GlobalContext& gctx, const std::string& package_name) {
    using namespace std::filesystem;
    
    std::vector<Target> targets;
    std::vector<path> main_target_paths;
    
    for (auto& entry : directory_iterator(gctx.cwd() / "src")) {
        if (entry.is_regular_file()) {
            main_target_paths.push_back(entry);
        } else if (entry.is_directory()) {
            if (entry.path().filename() == "bin") {
                auto binary_targets = infer_binary_targets(entry);
                if (!binary_targets) {
                    return std::unexpected(binary_targets.error());
                }

                ranges::move_back_range(targets, *binary_targets);
            } else {
                main_target_paths.push_back(entry);
            }
        } else {
            return std::unexpected(entry);
        }
    }

    if (!main_target_paths.empty()) {
        targets.emplace_back(package_name, main_target_paths);    
    }
    
    return targets;
}

class ManifestReaderState {
    const GlobalContext& _gctx;
    std::filesystem::path _manifest_path;
public:
    ManifestReaderState(const std::filesystem::path manifest_path,
        const GlobalContext& gctx) : _gctx{gctx}, _manifest_path{manifest_path} {}
    
    const GlobalContext& gctx() const { return _gctx; }
    const std::filesystem::path& manifest_path() const { return _manifest_path; }

    [[noreturn]] void fail(std::string_view message) const {
        bail("failed to parse manifest at `{}`\n\n{}", _manifest_path.string(), message);
    }
};

std::filesystem::path infer_target_path(ManifestReaderState& mrs, const std::string target_name) {
    using namespace std::filesystem;

    auto bin = mrs.manifest_path().parent_path() / "src/bin";
    
    auto source_file1 = (bin / target_name).replace_extension(".c");
    bool found_source1 = is_regular_file(source_file1);
    
    auto source_file2 = bin / target_name / "main.c";
    bool found_source2 = is_regular_file(source_file2);

    if (found_source1 && found_source2) {
        mrs.fail(cause("cannot infer path for `{0}` bin\n"
            "Cargo doesn't know which to use because multiple target files"
            " found at `src/bin/{0}/main.c` and `src/bin/{0}.c`.", target_name));
    } else if (found_source1) {
        return source_file1;
    } else if (found_source2) {
        return source_file2;
    }

    mrs.fail(cause("can't find `{0}` bin at `src/bin/{0}.c` or `src/bin/{0}/main.c`.\n"
        "Please specify bin.path if you want to use a non-default path.",
        target_name));
}

static Manifest read_manifest(const GlobalContext& gctx, const std::filesystem::path& manifest_path) {
    TomlManifest toml_manifest = serialize_toml(manifest_path);

    ManifestReaderState mrs{manifest_path, gctx};
    
    // TODO: Check deserialized manifest for package name instead
    std::string package_name = manifest_path.parent_path().filename();

    std::vector<Target> targets;
    if (toml_manifest.bin) {
        // TODO: Define structs for `toml_manifest` and `toml_target`
        auto& toml_bin_targets = *toml_manifest.bin;
        for (auto& toml_target : toml_bin_targets) {
            Target target {
                .name = *toml_target.name,
                .paths{},
            };
            
            if (toml_target.paths) {
                for (auto& path : *toml_target.paths) {
                    if (!exists(path)) {
                        // TODO: Implement logic when a path specified for a target doesn't exist
                    }
                }
                
                target.paths = *toml_target.paths;
            } else {
                target.paths.push_back(infer_target_path(mrs, target.name));
            }

            targets.emplace_back(std::move(target));
        }
    } else {
        auto inferred_targets = ::infer_targets(gctx, package_name);
        if (!inferred_targets) {
            bail("source file `{}` is not a regular file", inferred_targets.error().path().string());
        }

        ranges::move_back_range(targets, *inferred_targets);
    }

    if (targets.empty()) {
        mrs.fail(cause("no targets specified in the manifest\n"
            "  either src/lib.rs, src/main.rs, a [lib] section, or"
            " [[bin]] section must be present"));
    }

    return Manifest{
        std::move(toml_manifest),
        package_name,
        std::move(targets)
    };
}

bool filesystem::contains_manifest(const std::filesystem::path& dir) {
    auto manifest_path = dir / "Stim.toml";
    return std::filesystem::exists(manifest_path);
}

std::optional<std::filesystem::path> find_manifest(const std::filesystem::path& manifest_path) {
    std::filesystem::path dir;
    for (auto& component : manifest_path.parent_path()) {
        dir /= component;
        auto expected_manifest = dir / "Stim.toml";
        if (exists(expected_manifest)) {
            return expected_manifest;
        }
    }

    return {};
}

Workspace::Workspace(const std::filesystem::path& current_manifest,
    const GlobalContext& gctx)
:   _gctx{gctx} {
    
    _root_manifest = find_manifest(current_manifest);

    if (!std::filesystem::exists(current_manifest)) {
        if (!_root_manifest) {
            bail("could not find `Stim.toml` in `{}` or any parent directory",
                gctx.cwd().string());
        } else {
            _current_manifest = *_root_manifest;
        }
    } else {
        _current_manifest = current_manifest;
    }

    _packages.emplace(current_manifest, Package{
        read_manifest(gctx, current_manifest),
        current_manifest,
    });
}

const std::filesystem::path& GlobalContext::clang_path() const {
    static std::filesystem::path cached_path;

    if (!_compiler_path.empty()) {
        cached_path.clear();
        return _compiler_path;
    }

    // Cache path
    if (cached_path.empty()) {
        cached_path = filesystem::search_path("clang");
    }
    
    return cached_path;
}