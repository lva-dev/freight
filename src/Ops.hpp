#pragma once

#include <optional>
#include <string>
#include <vector>

#include "Workspace.hpp"

namespace freight::ops
{
struct InitOptions
{
	std::optional<std::string> path;
};

void init(GlobalContext& gctx, const InitOptions& opts);

struct NewOptions
{
	std::string path;
};

// NOLINTNEXTLINE(readability-identifier-naming)
void new_(GlobalContext& gctx, const NewOptions& opts);

struct BuildOptions
{
	bool release = false;
    std::optional<std::string> bin;
};

void build(GlobalContext& gctx, const BuildOptions& opts);

struct RunOptions
{
	bool release = false;
    std::optional<std::string> bin;
    std::vector<std::string> args;
};

void run(GlobalContext& gctx, const RunOptions& opts);
} // namespace freight::ops