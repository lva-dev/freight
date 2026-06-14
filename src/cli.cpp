#include "pch.h"

#include "cli.h"

#include <CLI11.hpp>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include "io.h"

using namespace freight::io;

namespace freight::cli {
struct Command::Impl {
	std::shared_ptr<CLI::App> app_;

	std::shared_ptr<CLI::App> app_ptr() {
		return app_;
	}

	CLI::App& app() {
		return *app_;
	}

	Impl(const std::string name)
		: app_ {std::make_shared<CLI::App>(name)} {
	}
};

ArgMatches::ArgMatches(Command& command) : command_ {&command} {
	assert(command.parsed());

	for (Command& sub : command.get_subcommands()) {
		if (sub.parsed()) {
			// subcommand_ = std::make_unique<SubCommand>(sub.get_name(), ArgMatches {sub});
		}
	}

	// TODO: Work
}

bool ArgMatches::flag(const std::string& name) const {
	return (command_->impl().app())[name]->as<bool>();
}

std::optional<std::string> ArgMatches::value(const std::string& name) const {
	return (command_->impl().app())[name]->as<std::string>();
}

Command::Command(const std::string& name)
	: impl_ {std::make_unique<Impl>(name)} {
	impl().app().require_subcommand(0, 1);
}

Command::Command(Command&& other)
	: impl_ {std::move(other.impl_)},
	  subcommands_ {std::move(other.subcommands_)} {
}

Command& Command::operator=(Command&& other) {
	impl_ = std::move(other.impl_);
	subcommands_ = std::move(other.subcommands_);
	return *this;
}

Command::~Command() {
}

ArgMatches Command::get_matches(int argc, char **argv) {
	try {
		impl().app().parse(argc, argv);
	} catch (const CLI::ParseError& e) {
		throw;
	}

	return ArgMatches {*this};
}

bool Command::parsed() {
	return impl().app().parsed();
}

std::span<Command> Command::get_subcommands() {
	return subcommands_;
}

const std::string& Command::get_name() {
	return impl().app().get_name();
}

void Command::print_help() {
	std::print("{}", impl().app().get_usage());
}

void Command::exit_with_runtime_error(const std::runtime_error& error) {
    auto parse_error = dynamic_cast<const CLI::ParseError*>(&error);
    if (parse_error) {
        std::exit(impl().app().exit(*parse_error));
    } else {
        err::fail("failed to parse arguments");
    }
}

Command&& Command::about(const std::string& description) {
	impl().app().description(description);
	return std::move(*this);
}

Command&& Command::arg_main_options() {
	impl().app().add_flag("-V,--version", "Print version info and exit");
	return std::move(*this);
}

Command&& Command::arg_new_options() {
	impl().app().add_option("PATH")->required();

	impl()
		.app()
		.add_flag("--name",
			"Set the resulting project name, defaults to the directory name");

	impl().app()
		.add_flag("--standard", "Standard to set for the project generated");

	return std::move(*this);
}

Command&& Command::arg_init_options() {
	impl().app().add_option("PATH")->default_val(".");

	impl()
		.app()
		.add_flag("--name",
			"Set the resulting project name, defaults to the directory name")
		->required();
        
	impl()
		.app()
		.add_flag("--standard", "Standard to set for the project generated")
		->required();

	return std::move(*this);
}

Command&& Command::arg_build_options() {
	impl().app().add_flag(
		"-r,--release", "Build artifacts in release mode, with optimizations");
	return std::move(*this);
}

Command&& Command::add_subcommand(Command&& command) {
    assert(&impl().app() != &command.impl().app());

	impl().app().add_subcommand(command.impl().app_ptr());
    
	subcommands_.emplace_back(std::move(command));
	return std::move(*this);
};
} // namespace freight::cli