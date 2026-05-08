#pragma once

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stb/stb_ds.h>
#include <cstddef>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

template<class... Args>
void emit_error(std::format_string<Args...> fmt, Args... args) {
    std::println(std::cerr, "\033[31merror:\033[39m {}", std::format(fmt, std::forward<Args>(args)...));
}

template<class... Args>
[[noreturn]] void bail(std::format_string<Args...> fmt, Args... args) {
    emit_error(fmt, std::forward<Args>(args)...);
    exit(1);
}

template<class... Args>
std::string cause(std::format_string<Args...> fmt, Args... args) {
    return std::format("Caused by:\n  {}", std::format(fmt, std::forward<Args>(args)...));
}

inline  std::string cause(std::string_view str) {
    return std::format("Caused by:\n  {}", str);
}

template<class... Args>
void status(const std::string_view status, std::format_string<Args...> fmt, Args... args) {
    std::print("   \033[32m{}\033[m {}\n", status, std::format(fmt, std::forward<Args>(args)...));
    std::cout.flush();
}

namespace io {
    bool write_file(const std::filesystem::path file, std::string_view content);

    /**
    * A handle to an anonymous file, that is, a memory-mapped file without a name in the
    * filesystem, making it accessible only through its file descriptor (which is owned
    * by the returned FdStream).
    */
    struct AnonymousFile {
    private:
        inline static constexpr int NO_FD = -1;
        int _fd = NO_FD;
        AnonymousFile(int fd) : _fd{fd} {}
    public:
        AnonymousFile() = default;
        static AnonymousFile create() {
            std::error_code errc;
            auto file = create(errc);
            if (errc) {
                throw std::system_error{errc};
            }

            return file;
        }
        static AnonymousFile create(std::error_code& errc);
        ~AnonymousFile();
        AnonymousFile(const AnonymousFile&) = delete;
        AnonymousFile& operator=(const AnonymousFile&) = delete;
        AnonymousFile(AnonymousFile&&);
        AnonymousFile& operator=(AnonymousFile&&);
        
        bool is_open() const { return _fd != NO_FD; }

        std::filesystem::path path() const;
    };
 
    /**
     * A handle to an unnamed or named pipe.
     */
    class Pipe {
    private:
        using Handle = std::variant<std::array<int, 2>, int>; 
        std::optional<Handle> _id;
        bool _named;
    public:
        Pipe() = default;
        static Pipe create();
        static Pipe create_named(const std::filesystem::path& path);
        ~Pipe();
        
        Pipe& operator=(const Pipe&) = delete;
        Pipe(const Pipe&) = delete;
        Pipe& operator=(Pipe&&) = default;
        Pipe(Pipe&& other) = default;
        
        bool is_open() const { return _id.has_value(); }
        bool is_unnamed() const { return is_open() && !_named; }
        bool is_named() const { return is_open() && _named; }
        
        std::ifstream read_end() const;
        std::ofstream write_end() const;
    };
}

namespace ds {
    /**
    * A null terminated dynamic array of of C strings.
    */
    class Strings {
        std::vector<char*> _data;
        public:
        using value_type = char*;
        using allocator_type = decltype(_data)::allocator_type;
        using pointer = char**;
        using const_pointer = const char*;
        using iterator = decltype(_data)::iterator;
        using const_iterator = decltype(_data)::const_iterator;
        
        Strings() {
            _data.push_back(nullptr);
        }
        
        Strings(const Strings& other) : Strings{} {
            for (char* str : other) {
                push_back(str);
            }
        }
        
        Strings(Strings&& other) {
            for (char* str : *this) {
                delete[] str;
            }
            
            _data = std::move(other._data);
            other._data.clear();
        }
        
        ~Strings() {
            for (char* str : *this) {
                delete[] str;
            }
        }
        
        std::size_t size() const { return _data.size() - 1; }
        
        pointer data() { return _data.data(); }
        const value_type *data() const { return _data.data(); }
        
        const_pointer operator [](std::size_t i) const { return _data[i]; }
        
        void set(std::size_t i, std::string_view str);
        
        void reserve(std::size_t count) { _data.reserve(count + 1); }
        
        void push_back(std::string_view str) { insert(size(), str); }
        void push_front(std::string_view str) { insert(0, str); }
        void insert(std::size_t i, std::string_view str);
        void erase(std::size_t i);

        iterator begin() { return _data.begin(); }
        const_iterator begin() const { return _data.begin(); }
        const_iterator cbegin() const noexcept { return _data.cbegin(); }
        iterator end() { return _data.end() - 1; }
        const_iterator end() const { return _data.end() - 1; }
        const_iterator cend() const noexcept { return _data.cend() - 1; }
    };
}

/**
 * Builder for subprocesses. Contains the path to the executable and the
 * arguments to pass to it.
 */
class ProcessBuilder {
    std::filesystem::path _path;
    bool _name_inferred;
    ds::Strings _args;
public:
    ProcessBuilder(const std::filesystem::path& path);

    void set_path(const std::filesystem::path& path);
    void set_name(const std::string& name);
    void add_arg(const std::string& arg);
    void infer_name();

    int start() const;
};

ProcessBuilder process_new(const char *path);
void process_add_arg(ProcessBuilder* pb, const char* arg);

int process_start(const ProcessBuilder* pb);

namespace filesystem {
    std::filesystem::path search_path(const char* name);
}

namespace ranges {
    template<class R1, class R2>
    void move_back_range(R1& dest, R2&& src) {
        dest.insert(dest.end(),
            std::make_move_iterator(src.begin()),
            std::make_move_iterator(src.end()));
    }
}
