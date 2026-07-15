#pragma once

#include <concepts>
#include <format>
#include <memory>
#include <string>
#include <utility>

#include <gsl/pointers>

namespace error {

template<class E>
std::string error_to_string(const E& value);

std::string error_to_string(const std::formattable<char> auto& value) {
    return std::format("{}", value);
}

template<class T>
concept ErrorConcept = requires(T err) {
    { error_to_string(err) } -> std::same_as<std::string>;
};

// struct ErrorBase {
//     template<ErrorConcept T>
//     T& downcast() {
//         dynamic_cast<const T*>(source.get())
//     }

//     template<ErrorConcept T>
//     bool is() {
//         return typeid(T) == source.type();
//     }
// };

struct [[nodiscard]] Error {
private:
    using Deleter = void(*)(void*);
    using ToStringFunc = std::string(*)(void*);
public:
    template<ErrorConcept T>
    Error(T&& error) : Error{Error::from(std::forward<T>(error))}{

    }
    
    template<ErrorConcept T>
    static Error from(T&& error) {
        Deleter deleter = [](void* obj){ delete gsl::owner<T*>{static_cast<T*>(obj)}; }; // NOLINT
        auto source = std::unique_ptr<void, Deleter>{new T{std::forward<T>(error)}, deleter};
        auto toStringFunc = [](void* err) { return error_to_string(*static_cast<T*>(err)); };
        return Error{std::move(source), toStringFunc};
    }

    template<ErrorConcept T>
    T* source() { return dynamic_cast<T*>(source_.get()); }

    template<ErrorConcept T>
    const T* as() const { return dynamic_cast<const T*>(source_.get()); }

    std::string to_string() const { return toStringFunc(source_.get()); }
private:
    std::unique_ptr<void, Deleter> source_;
    ToStringFunc toStringFunc;

    Error(std::unique_ptr<void, Deleter>&& source, ToStringFunc toStringFunc)
    : source_{std::move(source)}, toStringFunc{toStringFunc} {}
};

Error error_from(ErrorConcept auto&& error) {
    return Error::from(std::move(error));
}

}

template<>
struct std::formatter<error::Error> {
    auto format(const error::Error& obj, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", obj.to_string());
    }
};