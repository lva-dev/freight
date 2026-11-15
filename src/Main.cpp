#include "Pch.hpp"

#include <clap/Lexer/Lexer.hpp>
#include <concepts>
#include <filesystem>
#include <functional>
#include <generator>
#include <optional>
#include <print>
#include <string_view>
#include <type_traits>

#include "Error.hpp"
#include "Ops.hpp"
#include "Workspace.hpp"
#include "Support/Ranges.hpp"

using namespace freight;

namespace cli_util
{
static auto bail_with_no_such_command(std::string_view cmd)
{
	error::bail("no such command `{}`", cmd);
}

static auto bail_with_missing_required_arg(std::string_view arg)
{
	error::bail("the required argument '\033[36m{}\033[39m' was not provided", arg);
}

[[maybe_unused]] static auto bail_with_missing_required_subcommand(std::string_view cmd)
{
	error::bail(
		"'\033[32m{}\033[39m' requires a subcommand but one was not provided", cmd);
}

static auto bail_with_unexpected_argument(std::string_view arg)
{
	error::bail(std::format("unexpected argument '\033[36m{}\033[39m' found", arg));
};

static auto raw_args_to_generator(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) -> std::generator<clap::lexer::ParsedArg>
{
	return support::ranges::iter_optional([&] { return raw_args.next(arg_cursor); });
}

static auto short_flags_to_generator(clap::lexer::ShortFlags& short_flags)
	-> std::generator<char>
{
	return support::ranges::iter_optional([&] { return short_flags.next_flag(); });
}

static auto default_parse_long_callback(clap::lexer::RawArgs&,
	clap::lexer::ArgCursor&,
	const clap::lexer::LongFlag&) -> bool
{
	return false;
};

static auto
default_parse_short_callback(clap::lexer::RawArgs&, clap::lexer::ArgCursor&, char) -> bool
{
	return false;
};

static auto default_parse_pos_callback(clap::lexer::RawArgs&,
	clap::lexer::ArgCursor&,
	std::string_view) -> bool
{
	return false;
};

struct GenericParser
{
	using Lc = std::function<bool(clap::lexer::RawArgs&,
		clap::lexer::ArgCursor&,
		const clap::lexer::LongFlag&)>;
	using Sc = std::function<bool(clap::lexer::RawArgs&, clap::lexer::ArgCursor&, char)>;
	using Pc = std::
		function<bool(clap::lexer::RawArgs&, clap::lexer::ArgCursor&, std::string_view)>;

	Lc parse_long_callback = default_parse_long_callback;
	Sc parse_short_callback = default_parse_short_callback;
	Pc parse_pos_callback = default_parse_pos_callback;

	auto parse(clap::lexer::RawArgs& raw_args, clap::lexer::ArgCursor& arg_cursor) const;
private:
	auto try_parse_long(clap::lexer::RawArgs& raw_args,
		clap::lexer::ArgCursor& arg_cursor,
		const clap::lexer::ParsedArg& arg) const -> bool;

	auto try_parse_short(clap::lexer::RawArgs& raw_args,
		clap::lexer::ArgCursor& arg_cursor,
		const clap::lexer::ParsedArg& arg) const -> bool;
};

auto GenericParser::parse(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) const
{
	bool found_escape = false;
	for (auto arg : cli_util::raw_args_to_generator(raw_args, arg_cursor))
	{
		if (!found_escape)
		{
			if (arg.is_escape())
			{
				found_escape = true;
				continue;
			}

			if (try_parse_long(raw_args, arg_cursor, arg))
			{
				continue;
			}

			if (try_parse_short(raw_args, arg_cursor, arg))
			{
				continue;
			}
		}

		if (!std::invoke_r<bool>(
				parse_pos_callback, raw_args, arg_cursor, arg.to_value()))
		{
			bail_with_unexpected_argument(arg.to_value());
		}
	}
}

auto GenericParser::try_parse_long(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor,
	const clap::lexer::ParsedArg& arg) const -> bool
{
	if (auto as_long = arg.to_long(); as_long.has_value())
	{
		if (std::invoke_r<bool>(parse_long_callback, raw_args, arg_cursor, *as_long))
		{
			return true;
		}

		bail_with_unexpected_argument(std::format("--{}", as_long->name));
	}

	return false;
}

auto GenericParser::try_parse_short(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor,
	const clap::lexer::ParsedArg& arg) const -> bool
{
	if (auto as_short = arg.to_short(); as_short.has_value())
	{
		for (auto flag : cli_util::short_flags_to_generator(*as_short))
		{
			if (std::invoke_r<bool>(parse_short_callback, raw_args, arg_cursor, flag))
			{
				return true;
			}

			bail_with_unexpected_argument(std::format("-{}", flag));
		}
	}

	return false;
}
} // namespace cli_util

struct MainOptions
{
	bool help = false;
	std::optional<std::string> command;
};

static auto parse_main_options(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) -> MainOptions
{
	MainOptions opts {};

	for (auto arg : cli_util::raw_args_to_generator(raw_args, arg_cursor))
	{
		if (auto as_long = arg.to_long(); as_long.has_value())
		{
			if (as_long->name == "help")
			{
				opts.help = true;
			}
			else
			{
				cli_util::bail_with_unexpected_argument(
					std::format("--{}", as_long->name));
			}

			continue;
		}

		if (auto as_short = arg.to_short(); as_short.has_value())
		{
			for (auto flag : cli_util::short_flags_to_generator(*as_short))
			{
				if (flag == 'h')
				{
					opts.help = true;
				}
				else
				{
					cli_util::bail_with_unexpected_argument(std::format("-{}", flag));
				}
			}

			continue;
		}

		if (!opts.command.has_value())
		{
			opts.command = arg.to_value();
            break;
		}
	}

	return opts;
}

static auto parse_build_options(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) -> ops::BuildOptions
{
	ops::BuildOptions opts {};
	cli_util::GenericParser parser {
		.parse_long_callback =
			[&](auto&, auto&, const auto& flag) {
				if (flag.name == "release")
				{
					opts.release = true;
					return true;
				}

				return false;
			},
		.parse_short_callback =
			[&](auto&, auto&, auto flag) {
				if (flag == 'r')
				{
					opts.release = true;
					return true;
				}

				return false;
			},
	};

	parser.parse(raw_args, arg_cursor);
	return opts;
}

static auto parse_new_options(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) -> ops::NewOptions
{
	ops::NewOptions opts {};
	bool found_path = false;

	cli_util::GenericParser parser {
		.parse_pos_callback = [&](auto&, auto&, auto arg) {
			if (!found_path)
			{
				opts.path = arg;
				found_path = true;
				return true;
			}

			return false;
		},
	};

	parser.parse(raw_args, arg_cursor);

	if (!found_path)
	{
		cli_util::bail_with_missing_required_arg("<PATH>");
	}

	return opts;
}

static auto parse_init_options(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) -> ops::InitOptions
{
	ops::InitOptions opts {};

	cli_util::GenericParser parser {
		.parse_pos_callback = [&](auto&, auto&, auto arg) {
			if (!opts.path.has_value())
			{
				opts.path = arg;
				return true;
			}

			return false;
		},
	};

	parser.parse(raw_args, arg_cursor);

	return opts;
}

static auto parse_run_options(clap::lexer::RawArgs& raw_args,
	clap::lexer::ArgCursor& arg_cursor) -> ops::RunOptions
{
	ops::RunOptions opts {};
	cli_util::GenericParser parser {
		.parse_long_callback =
			[&](auto&, auto&, const auto& flag) {
				if (flag.name == "release")
				{
					opts.release = true;
					return true;
				}

				return false;
			},
		.parse_short_callback =
			[&](auto&, auto&, auto flag) {
				if (flag == 'r')
				{
					opts.release = true;
					return true;
				}

				return false;
			},
        .parse_pos_callback = [&](auto&, auto&, auto arg){
            opts.args.emplace_back(arg);
            return true;
        },
	};

	parser.parse(raw_args, arg_cursor);


	return opts;
}

auto print_help_and_exit()
{
	std::print(
		"Usage: freight [OPTIONS] [COMMAND]\n"
		"\n"
		"Options:\n"
		"  -V, --version                  Print version info and exit\n"
		"  -h, --help                     Print help\n"
		"\n"
		"Commands:\n"
		"    build, b    Compile the current package\n"
		"    new         Create a new freight package\n"
		"    init        Create a new freight package in an existing directory\n"
		"    run, r      Run a binary of the local package\n");
	std::exit(0);
}

auto main(int argc, char **argv) -> int
{
	std::span<char *> args_span {argv, static_cast<std::size_t>(argc)};
	auto raw_args = clap::lexer::RawArgs::create(args_span.subspan(1));
	auto arg_cursor = raw_args.cursor();

	auto main_opts = parse_main_options(raw_args, arg_cursor);
	if (main_opts.help || !main_opts.command.has_value())
	{
		print_help_and_exit();
	}

	auto gctx = GlobalContext::create(std::filesystem::current_path());
	auto& cmd = *main_opts.command;
	if (cmd == "build" || cmd == "b")
	{
		ops::build(gctx, parse_build_options(raw_args, arg_cursor));
	}
	else if (cmd == "new")
	{
		ops::new_(gctx, parse_new_options(raw_args, arg_cursor));
	}
	else if (cmd == "init")
	{
		ops::init(gctx, parse_init_options(raw_args, arg_cursor));
	}
	else if (cmd == "run" || cmd == "r")
	{
		ops::run(gctx, parse_run_options(raw_args, arg_cursor));
	}
	else if (cmd == "help")
	{
		print_help_and_exit();
	}
	else
	{
		cli_util::bail_with_no_such_command(cmd);
	}
}