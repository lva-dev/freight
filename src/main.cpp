#include "pch.h"

#include "cli.h"
#include "io.h"

int main(int argc, char **argv) {
	using namespace freight;
    using namespace freight::cli;
	using namespace freight::exec;

	auto global = cmd::global();
	ArgMatches matches;

	try {
		matches = global.get_matches(argc, argv);
	} catch (const std::runtime_error& e) {
        global.exit_with_runtime_error(e);
	}

	SubCommand* sub = matches.subcommand();
	if (!sub) {
		global.print_help();
		return 0;
	}

	GlobalContext gctx = GlobalContext::get();
	Exec exec = infer(sub->name);
	exec(gctx, matches);
}