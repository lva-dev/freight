#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace freight
{
struct Manifest
{
  public:
    std::string name;
    std::string standard;
    std::string version;

    Manifest() = default;
    Manifest(const Manifest&) = default;
    Manifest(Manifest&&) = default;
    Manifest& operator=(const Manifest&) = default;
    Manifest& operator=(Manifest&&) = default;

    static inline std::filesystem::path FILENAME = "Proj.toml";
};

} // namespace freight