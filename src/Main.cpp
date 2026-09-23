#include "Pch.hpp"

#include <filesystem>
#include <print>

#include "Error.hpp"
#include "Ops.hpp"
#include "Workspace.hpp"
#include "clap/Lexer/Lexer.hpp"

using namespace freight;

static auto no_such_command(std::string_view cmd) -> std::string
{
	return std::format("no such command `{}`", cmd);
}

static auto missing_required_arg(std::string_view arg) -> std::string
{
	return std::format(
		"the required argument '\033[36m{}\033[39m' was not provided", arg);
}

static auto missing_required_subcommand(std::string_view cmd) -> std::string
{
	return std::format(
		"'\033[32m{}\033[39m' requires a subcommand but one was not provided", cmd);
}

auto main(int argc, char **argv) -> int
{
	std::span<char *> argsSpan {argv, static_cast<std::size_t>(argc)};
	auto rawArgs = clap::lexer::RawArgs::create(argsSpan.subspan(1));
	auto argCursor = rawArgs.cursor();

	auto maybeName = rawArgs.next(argCursor);
	if (!maybeName.has_value())
	{
		error::bail("{}", missing_required_subcommand("freight"));
	}

	auto gctx = GlobalContext::create(std::filesystem::current_path());

	auto name = std::move(maybeName)->to_value();
	if (name == "build" || name == "b")
	{
		ops::build(gctx, ops::BuildOptions {});
	}
	else if (name == "new")
	{
		auto maybePath = rawArgs.next(argCursor);
		if (!maybePath.has_value())
		{
			error::bail("{}", missing_required_arg("<PATH>"));
		}

		ops::NewOptions opts {
			.path {std::move(maybePath)->to_value()},
		};

		ops::new_(gctx, opts);
	}
	else if (name == "init")
	{
		ops::init(gctx, ops::InitOptions {});
	}
	else if (name == "run" || name == "r")
	{
		ops::run(gctx, ops::RunOptions {});
	}
	else
	{
		error::bail("{}", no_such_command(name));
	}
}