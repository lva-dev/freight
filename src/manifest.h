#pragma once

#include <string>

namespace freight {
static inline constexpr const std::string MANIFEST_FILENAME = "Proj.toml";

struct Manifest {
	std::string name;
	std::string standard;
	std::string version;
	bool debug;

	Manifest() = default;
	Manifest(const Manifest&) = default;
	Manifest(Manifest&&) = default;
	Manifest& operator=(const Manifest&) = default;
	Manifest& operator=(Manifest&&) = default;
};
} // namespace freight