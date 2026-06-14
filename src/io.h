#pragma once

#include <cassert>
#include <filesystem>
#include <format>
#include <print>
#include <stdexcept>
#include <string_view>

namespace freight {
namespace io {
	using std::print;
	using std::println;

	/**
	 * @brief Prints a formatted string to stderr.
	 * Equivalent to `std::print(stderr, fmt,
	 * std::forward<Args>(args)...);`.
	 * @param fmt the format string
	 * @param args the format args
	 * \see std::print
	 */
	template<class... Args>
	inline void eprint(std::format_string<Args...> fmt, Args&&...args) {
		::std::print(stderr, fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Prints a formatted string to stderr.
	 * Equivalent to `std::println(stderr, fmt,
	 * std::forward<Args>(args)...);`.
	 * @param fmt the format string. see std::println()
	 * @param args the format args
	 * \see std::println
	 */
	template<class... Args>
	inline void eprintln(std::format_string<Args...> fmt, Args&&...args) {
		::std::println(stderr, fmt, std::forward<Args>(args)...);
	}

	/**
	 * @brief Prints an error message (terminated with a new line) with in
	 * the format "error: {message}".
	 * @param message the message to print
	 */
	inline void error(std::string_view message) {
		eprintln("\033[31merror:\033[m {}", message);
	}

	/**
	 * @brief Prints a warning message (terminated with a new line) to
	 * stderr.
	 * @param message the message to print
	 */
	inline void warning(const std::string_view& message) {
		::std::println(stderr, "\033[38;5;3mwarning\033[m: {}", message);
	}
} // namespace io

namespace err {
	inline constexpr const int FAILURE_EXIT_CODE = 1;

	class Failure {
	private:
	public:
		Failure() = default;
		Failure(const Failure&) = default;
		Failure(Failure&&) = default;
		Failure& operator=(const Failure&) = default;
		Failure& operator=(Failure&&) = default;

		~Failure() {
			std::logic_error {
				"exit() was not called for this freight::err::Failure::Failure "
				"object"};
		}

		Failure& msg(const std::string& message) {
			io::eprintln("\n{}", message);
			return *this;
		}

		Failure& cause(const std::string& cause) {
			io::eprintln("\nCaused by:\n  {}", cause);
			return *this;
		}

		Failure& more_info() {
			io::eprintln("\nFor more information, try '\033[36m--help\033[m'.");
			return *this;
		}

		Failure& usage(const std::string& usage) {
			io::eprintln(
				"\n\033[32mUsage:\033[m \033[36mfreight {}\033[m", usage);
			return *this;
		}

		[[noreturn]] void exit() {
			std::exit(FAILURE_EXIT_CODE);
		}
	};

	/**
	 * @brief Exits the program with exit code 1 after printing an error
	 * message (terminated with a new line) to stderr. After printing,
	 * invokes `std::exit(1)`.
	 * @param message the message to print
	 */
	inline Failure fail(std::string_view message) {
		io::error(message);
		return Failure();
	}

	/**
	 * @brief Exits the program with exit code 1 after printing an error
	 * message (terminated with a new line) to stderr. After printing,
	 * invokes `std::exit(1)`.
	 * @param message the message to print
	 */
	template<class... Args>
	inline Failure fail(std::format_string<Args...> fmt, Args&&...args) {
		io::error(std::format(fmt, std::forward<Args>(args)...));
		return Failure();
	}

	inline Failure missing_arg(std::string_view arg) {
		io::error("the following required arguments were not provided:");
		io::eprintln("  \033[36m{}\033[m", arg);
		return Failure();
	}

	inline Failure unexpected_arg(std::string_view arg) {
		io::error(
			std::format("unexpected argument '\033[33m{}\033[m' found", arg));
		return Failure();
	}

	inline Failure destination_exists(const std::filesystem::path& path) {
		using namespace std::filesystem;
		io::error(std::format(
			"destination `{}` already exists", absolute(path).string()));
		return Failure();
	}
} // namespace err
} // namespace freight