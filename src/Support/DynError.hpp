#pragma once

#include <gsl/pointers>

#include <any>
#include <cassert>
#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <print>
#include <string>
#include <type_traits>
#include <utility>

namespace support::error
{
// template<class E>
// std::string error_to_string(const E& value);

template<class Err>
concept ErrorConcept = requires(Err err) {
	{ error_to_string(err) } -> std::same_as<std::string>;
};

struct DynError
{
public:
	template<ErrorConcept Err>
		requires(!std::same_as<std::remove_cvref_t<Err>, DynError>)
	static auto create(Err&& error) -> DynError
	{
		return DynError {std::forward<Err>(error)};
	}

	template<ErrorConcept Err, class... Args>
	DynError(std::in_place_type_t<Err>, Args&&...args)
		: src {std::in_place_type<Err>, std::forward<Args>(args)...}
		, toStringFunc {any_error_to_string<Err>}
	{}

	~DynError() = default;
	DynError(const DynError&) = delete;
	auto operator=(const DynError&) -> DynError& = delete;
	DynError(DynError&&) = default;
	auto operator=(DynError&&) -> DynError& = default;

	template<ErrorConcept Err>
	auto as() noexcept -> Err *
	{
		return std::any_cast<Err>(&src);
	}

	template<ErrorConcept Err>
	auto as() const noexcept -> const Err *
	{
		return std::any_cast<Err>(&src);
	}

	template<ErrorConcept Err>
	[[nodiscard]] auto is() const -> bool
	{
		return typeid(Err) == src.type();
	}

	[[nodiscard]] auto to_string() const -> std::string
	{
		return toStringFunc(src);
	}
private:
	std::any src;
	std::string (*toStringFunc)(const std::any&);

	template<ErrorConcept Err>
	static auto any_error_to_string(const std::any& err) -> std::string
	{
		assert(err.type() == typeid(Err) &&
			   "There was a mismatch between Err and err.type()");
		return error_to_string(*std::any_cast<std::remove_cvref_t<Err>>(&err));
	}
};
} // namespace support::error

template<>
struct std::formatter<support::error::DynError>
{
	constexpr static auto parse(std::format_parse_context& ctx)
	{
		return ctx.begin();
	}

	static auto format(const support::error::DynError& obj, std::format_context& ctx) 
	{
		return std::format_to(ctx.out(), "{}", obj.to_string());
	}
};