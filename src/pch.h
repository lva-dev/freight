#pragma once

// Boost
#include <boost/algorithm/string/split.hpp>
// CLI11
#include <CLI11.hpp>
// Toml++
#include <toml++/toml.hpp>

// Standard Library
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_set>
#include <utility>
#include <vector>

// POSIX
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>