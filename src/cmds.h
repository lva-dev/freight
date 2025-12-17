#pragma once

#include <filesystem>
#include <optional>
#include <string>

namespace freight {
	struct BuildOptions {
		std::filesystem::path path;
		bool release;
	};

	void build(const BuildOptions& opts);

	struct NewOptions {
		std::filesystem::path path;
		std::string name;
		std::optional<std::string> standard = {};
		std::optional<std::string> version = {};
	};

	void new_(const NewOptions& opts);

    struct InitOptions {
		std::string name;
		std::optional<std::string> standard = {};
		std::optional<std::string> version = {};
	};

	void init(const InitOptions& opts);
} // namespace freight