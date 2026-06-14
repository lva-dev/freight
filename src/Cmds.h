#pragma once

#include "Util.h"

struct InitOptions {
    std::optional<std::string> path;
};

struct NewOptions {
    std::string path;
};

struct BuildOptions {
    bool release;
};

struct RunOptions {
    BuildOptions build_opts;
};

void exec_init(const InitOptions& opts);
void exec_new(const NewOptions& opts);
void exec_build(const BuildOptions& opts);
void exec_run(const RunOptions& opts);