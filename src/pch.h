#pragma once

// Standard Library
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

// Docopt
#include <docopt/docopt.h>

// Toml++
#include <toml++/toml.hpp>

// POSIX
#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>