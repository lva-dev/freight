#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace freight {
	static inline constexpr const std::string MANIFEST_FILENAME = "Proj.toml";

	struct Manifest {
		std::string name;
		std::string standard;
		std::string version;

		Manifest() = default;
		Manifest(const Manifest&) = default;
		Manifest(Manifest&&) = default;
		Manifest& operator=(const Manifest&) = default;
		Manifest& operator=(Manifest&&) = default;
	};
} // namespace freight