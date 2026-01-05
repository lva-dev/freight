#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "cmds.h"
#include "io.h"

namespace freight {
namespace cli {
	class ArgMatches;
	class Command;
	struct SubCommand;

	class ArgMatches {
	private:
		friend class Command;

		Command *command_;
		std::unique_ptr<SubCommand> subcommand_;

		explicit ArgMatches(Command& command);
	public:
		ArgMatches() = default;
		ArgMatches(ArgMatches&&) = default;
		ArgMatches& operator=(ArgMatches&&) = default;
		~ArgMatches() = default;

		bool flag(const std::string& str) const;
		std::optional<std::string> value(const std::string& str) const;

		SubCommand *subcommand() const {
			return subcommand_.get();
		}

		NewOptions to_new_options() const {
			return {
				.path = value("PATH").value(),
				.name = value("--name"),
				.standard = value("--standard"),
			};
		}

		InitOptions to_init_options() const {
			return {
				.path = value("PATH"),
				.name = value("--name"),
				.standard = value("--standard"),
			};
		}

		BuildOptions to_build_options() const {
			return {
				.release = flag("--release"),
			};
		}
	};

	struct SubCommand {
		std::string name;
		ArgMatches matches;
	};

	// class Arg {
	// private:
	// 	bool first_name_;
	// 	const std::string& names_;
	// public:
	// 	Arg(const std::string& id);
	// 	Arg&& longname(const std::string& name);
	// 	Arg&& shortname(char name);
	// 	Arg&& value_name(char name);
	// 	Arg&& required(bool yes);
	// };

	class Command {
	private:
		friend class ArgMatches;
		struct Impl;

		std::unique_ptr<Impl> impl_;
		std::vector<Command> subcommands_;

		Impl& impl() {
			return *impl_;
		}
	public:
		Command(const std::string& name);
		Command(const Command&) = delete;
		Command& operator=(const Command&) = delete;
		Command(Command&&);
		Command& operator=(Command&&);
		~Command();

		ArgMatches get_matches(int argc, char **argv);
		bool parsed();

		void print_help();
		void exit_with_runtime_error(const std::runtime_error& error);

		std::span<Command> get_subcommands();
		const std::string& get_name();

		Command&& about(const std::string& about);
		// Command&& arg(const Arg&& arg);
		Command&& add_subcommand(Command&& command);

		Command&& arg_main_options();
		Command&& arg_new_options();
		Command&& arg_init_options();
		Command&& arg_build_options();
		Command&& arg_run_options();
	};

	namespace cmd {
		inline Command build() {
			return Command {"build"}.arg_build_options();
		}

		inline Command init() {
			auto cmd = Command {"init"}.arg_init_options();
			return cmd;
		}

		inline Command new_() {
			auto cmd = Command {"new"}.arg_new_options();
			return cmd;
		}

		inline Command run() {
			return Command {"run"};
		}

		inline Command global() {
			return Command {"freight"}
				.arg_main_options()
				.add_subcommand(build())
				.add_subcommand(init())
				.add_subcommand(new_())
				.add_subcommand(run());
		}
	} // namespace cmd
} // namespace cli

namespace exec {
	using Exec = std::function<void(GlobalContext&, const cli::ArgMatches&)>;

	inline void new_(GlobalContext& gctx, const cli::ArgMatches& args) {
		// if () {
		// 	err::missing_arg("<PATH>").usage("new <PATH>").more_info().exit();
		// }

		// if (args.size() >= 2) {
		// 	err::unexpected_arg(args[1]).usage("new <PATH>").more_info().exit();
		// }

		auto opts = args.to_new_options();
		new_(gctx, opts);
	}

	inline void init(GlobalContext& gctx, const cli::ArgMatches& args) {
		// if (args.size() >= 1) {
		// 	err::unexpected_arg(args[0]).usage("new <PATH>").more_info().exit();
		// }

		auto opts = args.to_init_options();
		init(gctx, opts);
	}

	inline void build(GlobalContext& gctx, const cli::ArgMatches& args) {
		// if (args.size() >= 1) {
		// 	err::unexpected_arg(args[0]).usage("build").more_info().exit();
		// }

		auto opts = args.to_build_options();
		build(gctx, opts);
	}

	inline void run([[maybe_unused]] GlobalContext& gctx,
		[[maybe_unused]] const cli::ArgMatches& args) {
		err::fail("command 'run' has not been implemented yet");
	}

	inline Exec infer(const std::string& cmd) {
		if (cmd == "new") {
			return new_;
		} else if (cmd == "init") {
			return init;
		} else if (cmd == "build") {
			return build;
		} else if (cmd == "run") {
			return run;
		}

		err::fail("no such command: `{}`", cmd).exit();
	}
} // namespace exec

inline const std::string& usage() {
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
} // namespace freight