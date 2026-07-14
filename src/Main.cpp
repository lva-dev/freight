#include "Pch.h"

#include "Cmds.h"
#include "Util.h"

#include <cstddef>
#include <deque>
#include <functional>
#include <optional>
#include <print>
#include <span>
#include <string_view>

namespace cli {

class StringDeque {
public:
    void push(const std::string& str) { deque.push_front(str); }

    std::optional<std::string> pop() {
        if (deque.empty()) {
            return {};
        } else {
            auto arg = std::move(deque.front());
            deque.pop_front();
            return arg;
        }
    }

    [[nodiscard]] bool is_empty() const { return deque.empty(); }
private:
    std::deque<std::string> deque;
};

enum class MatchOptResult {
    Done,
    Match,
    None,
};

enum class MatchArgResult {
    Done,
    Continue,
};

struct Command {
public:
    Command() = default;
    
    virtual ~Command() = default;
    Command(const Command&) = default;
    Command& operator=(const Command&) = default;
    Command(Command&&) = default;
    Command& operator=(Command&&) = default;

    void Execute(StringDeque& args);
protected:
    virtual void ExecFunc(StringDeque& args) = 0;

    virtual MatchOptResult MatchOpt([[maybe_unused]] std::string_view arg) {
        return MatchOptResult::None;
    }
    
    virtual MatchArgResult MatchArg([[maybe_unused]] std::string_view arg) {
        return MatchArgResult::Continue;
    }
private:
    bool foundDoubleDash = false;
};

void Command::Execute(StringDeque& args) {
    for (auto argOpt = args.pop(); argOpt.has_value(); argOpt = args.pop()) {
        auto& arg = *argOpt;
        
        if (!foundDoubleDash && arg == "--") {
            foundDoubleDash = true;
        }
        
        bool matchedArg = false;
        if (!foundDoubleDash) {
            auto result = MatchOpt(arg);
            if (result == MatchOptResult::Done) {
                break;
            }

            matchedArg = result == MatchOptResult::None;
        }

        if (foundDoubleDash || matchedArg) {
            auto result = MatchArg(arg);
            if (result == MatchArgResult::Done) {
                break;
            }

            assert(result == MatchArgResult::Continue);
        }
    }

    ExecFunc(args);
}

}

using namespace cli;

void print_for_more_information() {
    std::println("For more information, try '\033[35m--help\033[39m'.");
}

[[noreturn]] void bail_with_unexpected_arg(std::string_view arg) {
    print_error("unexpected argument '\033[33m{}\033[39m' found", arg);
    std::exit(1);
}

[[noreturn]] void bail_with_missing_arg(std::string_view arg) {
    print_error("the required argument '\033[35m{}\033[39m' was not provided", arg);
    std::exit(1);
}

[[noreturn]] void bail_with_no_such_command(std::string_view arg) {
    print_error("no such command `{}`\n", arg);
    print_for_more_information();
    std::exit(1);
}

class NewCommand final : public Command {
public:
    NewCommand() = default;
private:
    std::optional<std::string> path = {};

    void ExecFunc(StringDeque&) override {
        if (!path.has_value()) {
            bail_with_missing_arg("<PATH>");
        }

        NewOptions opts {
            .path = std::move(*path),
        };

        exec_new(opts);
    }

    MatchArgResult MatchArg(std::string_view arg) override {
        if (!path.has_value()) {
            path = std::move(arg);
            return MatchArgResult::Done;
        } else {
            bail_with_unexpected_arg(arg);
        }
    }
};

class BuildCommand final : public Command {
public:
    BuildCommand() = default;
private:
    void ExecFunc(StringDeque&) override {
        BuildOptions opts {
            .release = false,
        };

        exec_build(opts);
    }
};

class InitCommand final : public Command {
public:
    InitCommand() = default;
private:
    void ExecFunc(StringDeque&) override {
        InitOptions opts {
            .path = {},
        };

        exec_init(opts);
    }
};

class RunCommand final : public Command {
public:
    RunCommand() = default;
private:
    void ExecFunc(StringDeque&) override {
        RunOptions opts {
            .build_opts = {},
        };

        exec_run(opts);
    }
};

class MainCommand final : public Command {
public:
    MainCommand() = default;
private:
    bool foundHelp = false;
    bool foundVersion = false;
    std::optional<std::string> command = {};
    
    MatchOptResult MatchOpt(std::string_view arg) override {
        if (!arg.starts_with('-')) {
            return MatchOptResult::None;
        }

        if (!foundHelp && (arg == "-h" || arg == "--help")) {
            foundHelp = true;
        } else if (!foundVersion && (arg == "-v" || arg == "--version")) {
            foundVersion = true;
        } else {
            return MatchOptResult::None;
        }
        
        return MatchOptResult::Match;
    }

    MatchArgResult MatchArg(std::string_view arg) override {
        command = std::move(arg);
        return MatchArgResult::Done;
    }

    void ExecFunc(StringDeque& args) override {
        if (foundHelp) {
            PrintHelp();
            std::exit(0);
        } else if (foundVersion) {
            PrintVersion();
            std::exit(0);
        }

        if (command.has_value()) {
            const auto& cmd = *command;
            if (cmd == "build" || cmd == "b") {
                return BuildCommand{}.Execute(args);
            } else if (cmd == "new") {
                return NewCommand{}.Execute(args);
            } else if (cmd == "init") {
                return InitCommand{}.Execute(args);
            } else if (cmd == "run" || cmd == "r") {
                return RunCommand{}.Execute(args);
            } else {
                bail_with_no_such_command(cmd);
            }
        } else {
            PrintHelp();
            std::exit(0);
        }
    }

    static void PrintHelp() {
        std::println("Help");
    }

    static void PrintVersion() {
        std::println("freight 0.1.0");
    }
};

int main(int argc, char **argv)
{
    StringDeque argDeque;
    auto args = std::span<char*>{argv, static_cast<std::size_t>(argc)};
    for (std::size_t i = 1; i < args.size(); i++) {
        argDeque.push(args[i]);
    }

    MainCommand mainCmd;
    mainCmd.Execute(argDeque);
}