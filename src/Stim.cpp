#include "Pch.h"

#include "Cmds.h"
#include "Util.h"

#include <clipp.h>

enum class CommandId {
    NEW,
    INIT,
    BUILD,
    RUN,
};

struct CommandOpts {
    CommandId id;
    bool help;
    bool version;
    NewOptions new_;
    InitOptions init;
    BuildOptions build;
    RunOptions run;
};

auto cli_new(CommandOpts& opts) {
    return clipp::command("new").set(opts.id, CommandId::NEW),
        clipp::value("PATH", opts.new_.path);
}

auto cli_init(CommandOpts& opts) {
    return clipp::command("init").set(opts.id, CommandId::INIT),
        clipp::opt_value("PATH", opts.init.path);
}

auto cli_build(CommandOpts& opts) {
    return clipp::command("build").set(opts.id, CommandId::BUILD),
        clipp::option("--release", "-r").set(opts.build.release);
}

auto cli_run(CommandOpts& opts) {
    return clipp::command("run").set(opts.id, CommandId::RUN),
        clipp::option("--release", "-r").set(opts.run.build_opts.release);
}

[[noreturn]] void bail_missing_arguments(std::span<std::string> args) {
    std::string message;
    for (auto& arg : args) {
        message += std::format("  {}\n", arg);
    }

    bail("the following required arguments were not provided:\n{}", message);
};

int main(int argc, char **argv)
{
    CommandOpts opts;
    auto cli = ((cli_init(opts) | cli_new(opts) | cli_build(opts) | cli_run(opts)),
        clipp::option("--help", "-h").set(opts.help),
        clipp::option("--version", "-v").set(opts.version));
        
    auto result = clipp::parse(argc, argv, cli);
    if (!result) {
        if (!result.missing().empty()) {
            static auto get_label = [](decltype(result.missing().front())& arg){
                return clipp::debug::doc_label(*arg.param());
            };
    
            auto s = std::ranges::to<std::vector>(
                std::views::transform(result.missing(), get_label));
                
            bail_missing_arguments(s);
        }
    }
    
    switch (opts.id) {
        case CommandId::INIT:
            exec_init(opts.init);
            break;
        case CommandId::NEW:
            exec_new(opts.new_);
            break;
        case CommandId::BUILD:
            exec_build(opts.build);
            break;
        case CommandId::RUN:
            exec_run(opts.run);
            break;
    }
}