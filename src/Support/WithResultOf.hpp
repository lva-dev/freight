#pragma once

#include <utility>

namespace support::util
{
template<class F>
class WithResultOf {
public:
    using T = decltype(std::declval<F&&>()());

    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved)
    explicit WithResultOf(F&& f) : f(std::forward<F>(f)) {}

    WithResultOf(const WithResultOf&) = delete;
    WithResultOf& operator=(const WithResultOf&) = delete;
    WithResultOf(WithResultOf&&) = delete;
    WithResultOf& operator=(WithResultOf&&) = delete;
    ~WithResultOf() = default;
    
    operator T() { return f(); }
private:
    F&& f;
};
}