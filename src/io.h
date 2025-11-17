#pragma once

#include <cassert>
#include <format>
#include <print>
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
			std::print(stderr, fmt, std::forward<Args>(args)...);
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
			std::println(stderr, fmt, std::forward<Args>(args)...);
		}

		/**
		 * @brief Prints an error message (terminated with a new line) with in
		 * the format "error: {message}".
		 * @param message the message to print
		 */
		inline void error(std::string_view message) {
			eprintln("\033[31merror\033[m: {}", message);
		}

		/**
		 * @brief Prints a warning message (terminated with a new line) to
		 * stderr.
		 * @param message the message to print
		 */
		inline void warning(const std::string_view& message) {
			std::println(stderr, "\033[38;5;3mwarning\033[m: {}", message);
		}
	} // namespace io

	namespace err {
		inline constexpr const int FAILURE_EXIT_CODE = 1;

		class Failure {
		private:
			bool _called_exit = false;
		public:
			Failure() = default;
			Failure(const Failure&) = default;
			Failure(Failure&&) = default;
			Failure& operator=(const Failure&) = default;
			Failure& operator=(Failure&&) = default;

			~Failure() {
#ifndef NDEBUG
				if (!_called_exit)
					assert(
						"Failure.exit() was not called for this Failure "
						"object");
#endif
			}

			Failure& cause(std::string_view cause) {
				io::eprintln("\nCaused by:\n  {}", cause);
				return *this;
			}

			[[noreturn]] void exit() {
#ifndef NDEBUG
				_called_exit = true;
#endif
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
	} // namespace err
} // namespace freight