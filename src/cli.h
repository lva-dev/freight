#pragma once

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <string_view>
#include <vector>

#include "cmds.h"
#include "io.h"

namespace freight {
	class Cli {
	private:
		const std::string& usage() const {
			static const std::string HELP =
				"\033[32mUsage:\033[m \033[35mfreight [options] "
				"[COMMAND]\033[m\n"
				"\n"
				"\033[32mOptions:\033[m\n"
				"  \033[35m-V\033[m, \033[35m--version\033[m   Print version "
				"info and exit\n"
				"  \033[35m-h\033[m, \033[35m--help\033[m      Print help\n"
				"\n"
				"\033[32mCommands:\033[m\n"
				"  \033[35mbuild\033[m, \033[35mb\033[m   Compile the current "
				"project\n"
				"  \033[35mnew\033[m        Create a new freight project\n"
				"  \033[35minit\033[m       Create a new freight project in an "
				"existing directory\n"
				"  \033[35mrun\033[m, \033[35mr\033[m     Run a binary of the "
				"local project";
			return HELP;
		}

		void print_usage() const { io::println("{}", usage()); }

		void handle_terminating_options(std::vector<std::string_view> args,
			std::size_t& arg_index) {

			for (; arg_index < args.size(); arg_index++) {
				auto arg = args[arg_index];

				if (!arg.starts_with("-")) {
					return;
				}

				if (arg == "--help" || arg == "-h") {
					print_usage();
					exit(0);
				}

				if (arg == "--version" || arg == "-V") {
					io::println("0.0.1");
					exit(0);
				}
			}
		}

		std::string_view get_subcommand(std::vector<std::string_view>& args,
			std::size_t& arg_index) {
			bool found_skip = false;
			for (; arg_index < args.size(); arg_index++) {
				std::string_view arg = args[arg_index];
                
                // TODO: transfer this logic into handle_terminating_options
				bool definitely_not_an_option =
					!found_skip && (!arg.starts_with("-") || arg == "-");
				if (definitely_not_an_option) {
					arg_index++;
					return arg;
				}

				if (!found_skip && arg == "--") {
					found_skip = true;
					continue;
				}

				if (arg.starts_with("--")) {
					// long option
					err::unexpected_arg(arg).more_info().exit();
				} else {
					// short option
					err::unexpected_arg(arg.substr(0, 2)).more_info().exit();
				}
			}

			print_usage();
			exit(0);
		}

		void invoke_new(std::vector<std::string_view>& args,
			std::size_t& arg_index) {
			using namespace std::filesystem;

			if (arg_index >= args.size()) {
				err::missing_arg("<PATH>")
					.usage("new <PATH>")
					.more_info()
					.exit();
			}

			if (arg_index + 1 < args.size()) {
				err::unexpected_arg(args[arg_index + 1])
					.usage("new <PATH>")
					.more_info()
					.exit();
			}

			std::filesystem::path path = args[arg_index++];
			std::string name = path.filename();

			io::println("    \033[32mCreating\033[m binary `{}` project", name);

			if (exists(path)) {
				err::destination_exists(path)
					.msg("Use `freight init` to initialize the directory")
					.exit();
			}

			std::optional<std::string> standard;
			std::optional<std::string> version;

			NewOptions opts {path, name, standard, version};

			new_(opts);
		}

		void invoke_init(std::vector<std::string_view>& args,
			std::size_t& arg_index) {
			err::fail("command 'init' has not been implemented yet");
		}

		void invoke_build(std::vector<std::string_view>& args,
			std::size_t& arg_index) {
			err::fail("command 'build' has not been implemented yet");
		}

		void invoke_run(std::vector<std::string_view>& args,
			std::size_t& arg_index) {
			err::fail("command 'run' has not been implemented yet");
		}
	public:
		void start(int argc, char **argv) {
			std::vector<std::string_view> args = {argv + 1, argv + argc};

			std::size_t arg_i = 0;

			handle_terminating_options(args, arg_i);
			std::optional<std::string_view> subcommand =
				get_subcommand(args, arg_i);

			if (subcommand == "new") {
				invoke_new(args, arg_i);
			} else if (subcommand == "init") {
				invoke_init(args, arg_i);
			} else if (subcommand == "build" || subcommand == "b") {
				invoke_build(args, arg_i);
			} else if (subcommand == "run" || subcommand == "r") {
				invoke_run(args, arg_i);
			} else {
				err::fail("no such command: `{}`", subcommand.value()).exit();
			}
		}
	};
} // namespace freight