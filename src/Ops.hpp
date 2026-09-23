#pragma once

#include <optional>
#include <string>

#include "Workspace.hpp"

namespace freight::ops {
struct InitOptions
{
	std::optional<std::string> path;
};

void init(GlobalContext& gctx, const InitOptions& opts);

struct NewOptions
{
	std::string path = {};
};

void new_(GlobalContext& gctx, const NewOptions& opts);

struct BuildOptions
{
    bool release = false;
};

void build(GlobalContext& gctx, const BuildOptions& opts);

struct RunOptions
{
	BuildOptions build_opts = {};
};

void run(GlobalContext& gctx, const RunOptions& opts, std::span<std::string> args = {});
} // namespace freight::ops