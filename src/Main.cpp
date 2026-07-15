#include "Pch.h"

#include <any>
#include <cstddef>
#include <deque>
#include <expected>
#include <format>
#include <functional>
#include <optional>
#include <print>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "Cmds.h"
#include "Support/Error.h"
#include "Support/Util.h"

namespace cli
{
// struct Error {

// };

// template<>
// std::string error::error_to_string<Error>(const Error& error) {
//     return "";
// }

inline static const std::string MORE_INFO = "For more information, try '\033[36m--help\033[39m'.";

std::string error_unexepected_arg(std::string_view arg)
{
	return std::format("unexpected argument '\033[33m{}\033[39m' found", arg);
}

std::string error_missing_arg(std::string_view arg)
{
	return std::format(
		"the required argument '\033[36m{}\033[39m' was not provided",
		arg);
}

std::string error_no_such_command(std::string_view arg)
{
	return std::format("no such command `{}`", arg);
}

template<class T> using Expected = std::expected<T, error::Error>;

class StringDeque
{
public:
	void push_back(const std::string& str)
	{
		deque.push_back(str);
	}

	std::optional<std::string> pop_front()
	{
		if (deque.empty())
		{
			return {};
		}
		else
		{
			auto arg = std::move(deque.front());
			deque.pop_front();
			return arg;
		}
	}

	[[nodiscard]] bool is_empty() const
	{
		return deque.empty();
	}
private:
	std::deque<std::string> deque;
};

namespace detail
{
	struct Matched
	{
		std::any value;
	};

	template<class T> struct Many
	{
		std::vector<T> values;
	};
} // namespace detail

struct Matches
{
public:
	bool get_flag(const std::string& id) const
	{
		const bool *value = get_one<bool>(id);
		if (value == nullptr)
		{
			throw std::invalid_argument {
				std::format("Invalid match type: Match '{}' is not Action::SetTrue "
							"or Action::SetFalse",
					id)};
		}
		else
		{
			return *value;
		}
	}

	template<class T> const T *get_one(const std::string& id) const
	{
		return std::any_cast<T>(&matches.at(id).value);
	}

	template<class T> const auto get_many(const std::string& id) const
	{
		using Many = detail::Many<T>;
		using View = decltype(std::views::all(Many {}));
		if (matches.contains(id))
		{
			Many *values = std::any_cast<Many>(&matches.at(id).value);
			if (values == nullptr)
			{
				throw_invalid_match_type<T>(id);
			}
			else
			{
				return std::optional<View>(std::views::all(values->values));
			}
		}
		else
		{
			return std::optional<View> {};
		}
	}
public:
	std::unordered_map<std::string, detail::Matched> matches;

	template<class T> [[noreturn]] void throw_invalid_match_type(const std::string& id)
	{
		assert(matches.contains(id));
		throw std::invalid_argument {
			std::format("Invalid match type: Match '{}' contains a '{}', not a '{}'",
				id,
				matches.at(id).value.type().name(),
				typeid(T).name())};
	}

	void add_flag(const std::string& id, bool value)
	{
		matches.try_emplace(id, detail::Matched {.value = std::make_any<bool>(value)});
	}

	template<class T> void add_one(const std::string& id, T&& value)
	{
		matches.try_emplace(
			id, detail::Matched {.value = std::make_any<T>(std::forward<T>((value)))});
	}

	template<class T> void add_many(const std::string& id, std::vector<T>&& values)
	{
		matches.try_emplace(id,
			detail::Matched {.value = std::make_any<detail::Many<T>>(std::move(values))});
	}
};

enum class MatchOptResult
{
	Match,
	Done,
	UnexpectedArg,
};

enum class MatchArgResult
{
	Match,
	Done,
	UnexpectedArg,
};

struct CommandParser
{
public:
	CommandParser() = default;
	virtual ~CommandParser() = default;
	CommandParser(const CommandParser&) = default;
	CommandParser& operator=(const CommandParser&) = default;
	CommandParser(CommandParser&&) = default;
	CommandParser& operator=(CommandParser&&) = default;

	Expected<void> parse(StringDeque& args);
protected:
	virtual Expected<void> execute(StringDeque& args) = 0;

	virtual MatchOptResult match_opt([[maybe_unused]] std::string_view arg,
		[[maybe_unused]] bool isLong)
	{
		return MatchOptResult::UnexpectedArg;
	}

	virtual MatchArgResult match_arg([[maybe_unused]] const std::string& arg)
	{
		return MatchArgResult::UnexpectedArg;
	}
private:
	bool foundDoubleDash = false;
};

Expected<void> CommandParser::parse(StringDeque& args)
{
	for (auto argOpt = args.pop_front(); argOpt.has_value(); argOpt = args.pop_front())
	{
		const auto& arg = *argOpt;

		if (!foundDoubleDash && arg == "--")
		{
			foundDoubleDash = true;
			continue;
		}

		if (!foundDoubleDash)
		{
			std::optional<MatchOptResult> result;
			if (arg.starts_with("--"))
			{
				result = match_opt(std::string_view {arg}.substr(2), true);
			}
			else if (arg.starts_with('-'))
			{
				if (arg.size() == 2)
				{
					result = match_opt(std::string_view {arg}.substr(1), false);
				}
				else if (arg.size() > 2)
				{
					return std::unexpected<error::Error>(
						std::format("{}\n\n\033[33mnote:\033[39m grouped options "
									"are not allowed\n\n{}",
							error_unexepected_arg(arg),
							MORE_INFO));
				}
			}

			if (result)
			{
				if (result == MatchOptResult::Done)
				{
					break;
				}
				else if (result == MatchOptResult::Match)
				{
					continue;
				}
				else if (result == MatchOptResult::UnexpectedArg)
				{
					return std::unexpected<error::Error>(
						std::format("{}\n\n", error_unexepected_arg(arg), MORE_INFO));
				}
			}
		}

		auto result = match_arg(arg);
		if (result == MatchArgResult::Done)
		{
			break;
		}
		else if (result == MatchArgResult::UnexpectedArg)
		{
			return std::unexpected<error::Error>(
				std::format("{}\n\n", error_unexepected_arg(arg), MORE_INFO));
		}
	}

	return execute(args);
}
} // namespace cli

using namespace cli;

class NewParser final : public CommandParser
{
public:
	NewParser() = default;
private:
	std::optional<std::string> path = {};

	Expected<void> execute(StringDeque&) override
	{
		if (!path.has_value())
		{
			return std::unexpected<error::Error>(std::format("{}\n\n", error_missing_arg("<PATH>"), MORE_INFO));
		}

		NewOptions opts {
			.path = std::move(*path),
		};

		exec_new(opts);
		return {};
	}

	MatchArgResult match_arg(const std::string& arg) override
	{
		if (!path.has_value())
		{
			path = std::move(arg);
			return MatchArgResult::Match;
		}
		else
		{
			return MatchArgResult::UnexpectedArg;
		}
	}
};

class BuildParser final : public CommandParser
{
public:
	BuildParser() = default;
private:
	Expected<void> execute(StringDeque&) override
	{
		BuildOptions opts {
			.release = false,
		};

		exec_build(opts);
		return {};
	}
};

class InitParser final : public CommandParser
{
public:
	InitParser() = default;
private:
	Expected<void> execute(StringDeque&) override
	{
		InitOptions opts {
			.path = {},
		};

		exec_init(opts);
		return {};
	}
};

class RunParser final : public CommandParser
{
public:
	RunParser() = default;
private:
	Expected<void> execute(StringDeque&) override
	{
		RunOptions opts {
			.build_opts = {},
		};

		exec_run(opts);
		return {};
	}
};

class MainParser final : public CommandParser
{
public:
	MainParser() = default;
private:
	bool foundHelp = false;
	bool foundVersion = false;
	std::optional<std::string> command = {};

	MatchOptResult match_opt(std::string_view arg, bool isLong) override
	{
		if (!foundHelp && ((!isLong && arg == "h") || (isLong && arg == "help")))
		{
			foundHelp = true;
		}
		else if (!foundVersion &&
				 ((!isLong && arg == "v") || (isLong && arg == "version")))
		{
			foundVersion = true;
		}
		else
		{
			return MatchOptResult::UnexpectedArg;
		}

		return MatchOptResult::Match;
	}

	MatchArgResult match_arg(const std::string& arg) override
	{
		command = std::move(arg);
		return MatchArgResult::Done;
	}

	Expected<void> execute(StringDeque& args) override
	{
		if (foundHelp)
		{
			print_help();
			std::exit(0);
		}
		else if (foundVersion)
		{
			print_version();
			std::exit(0);
		}

		if (command.has_value())
		{
			const auto& cmd = *command;
			if (cmd == "build" || cmd == "b")
			{
				return BuildParser {}.parse(args);
			}
			else if (cmd == "new")
			{
				return NewParser {}.parse(args);
			}
			else if (cmd == "init")
			{
				return InitParser {}.parse(args);
			}
			else if (cmd == "run" || cmd == "r")
			{
				return RunParser {}.parse(args);
			}
			else
			{
				return std::unexpected(std::format("{}\n\n{}", error_no_such_command(cmd), MORE_INFO));
			}
		}
		else
		{
			print_help();
			std::exit(0);
		}
	}

	static void print_help()
	{
		std::println("Help");
	}

	static void print_version()
	{
		std::println("freight 0.1.0");
	}
};

int main(int argc, char **argv)
{
	StringDeque argDeque;
	auto args = std::span<char *> {argv, static_cast<std::size_t>(argc)};
	for (std::size_t i = 1; i < args.size(); i++)
	{
		argDeque.push_back(args[i]);
	}

	MainParser mainCmd;
	auto result = mainCmd.parse(argDeque);
	if (!result.has_value())
	{
		print_error("{}", result.error().to_string());
		std::exit(1);
	}
}