#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace freight {
class GlobalContext {
private:
	GlobalContext() = default;
public:
	GlobalContext(const GlobalContext&) = delete;
	GlobalContext& operator=(const GlobalContext&) = delete;
	GlobalContext(GlobalContext&) = default;
	GlobalContext& operator=(GlobalContext&&) = default;

	static GlobalContext get() {
		return GlobalContext();
	};

	std::filesystem::path cwd() {
		return std::filesystem::current_path();
	}
};

struct BuildOptions {
	bool release;
};

void build(GlobalContext& gctx, const BuildOptions& opts);

struct NewOptions {
	std::filesystem::path path;
	std::optional<std::string> name = {};
	std::optional<std::string> standard = {};
};

void new_(GlobalContext& gctx, const NewOptions& opts);

struct InitOptions {
	std::optional<std::filesystem::path> path = {};
	std::optional<std::string> name = {};
	std::optional<std::string> standard = {};
};

void init(GlobalContext& gctx, const InitOptions& opts);
} // namespace freight