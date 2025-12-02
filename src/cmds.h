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

	void init(const NewOptions& opts);
} // namespace freight