#pragma once

#include <docopt/docopt.h>
#include <iostream>
#include <string>

class Cli {
private:
	const std::string& get_usage_for_docopt() {
		static const std::string HELP =
			R"(Usage: freight [options] [command]
Options:
  -V, --version
  -h, --help)";
		return HELP;
	}

	const std::string& usage() {

		static const std::string HELP =
			R"(Usage: freight [options] [COMMAND]
Options:
  -V, --version   Print version info and exit
  -h, --help      Print help
  
Commands:
  build, b   Compile the current project
  new        Create a new freight project
  init       Create a new freight project in an existing directory)";

		return HELP;
	}
public:
	void start(int argc, char **argv) {
		auto args = docopt::docopt_parse(
			usage(), {argv + 1, argv + argc}, false, "", true);

		for (auto const& arg : args) {
			std::cout << arg.first << ": " << arg.second << std::endl;
		}

		auto command = args["[COMMAND]"];
	}
};