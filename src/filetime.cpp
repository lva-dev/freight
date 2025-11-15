#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>

// std::filesystem::last_write_time

namespace nano
{
template <class Clock> const std::chrono::time_point<Clock> from_nanoticks(std::uint64_t ticks);
template <class Clock> std::uint64_t as_nanoticks(const std::chrono::time_point<Clock>& point);

} // namespace nano

template <class Clock>
const std::chrono::time_point<Clock> nano::from_nanoticks(std::uint64_t ticks)
{
    using namespace std::chrono;
    return time_point<Clock>{nanoseconds{ticks}};
}

template <class Clock> std::uint64_t nano::as_nanoticks(const std::chrono::time_point<Clock>& point)
{
    using namespace std::chrono;
    return duration_cast<nanoseconds>(point.time_since_epoch()).count();
}

namespace files
{
const std::filesystem::path& target_directory()
{
    static const std::filesystem::path TARGET_DIRECTORY =
        std::filesystem::current_path() / "target";
    return TARGET_DIRECTORY;
}

enum class Config
{
    Debug,
    Release
};

std::filesystem::path config_directory(Config config)
{
    if (config == Config::Debug)
    {
        return target_directory() / "debug";
    }
    else if (config == Config::Release)
    {
        return target_directory() / "release";
    } else
    {
        throw std::invalid_argument("Unrecognized config");
    }
}

} // namespace files