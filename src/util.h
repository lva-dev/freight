#pragma once
#include <stdarg.h>
#include <stb/stb_ds.h>

#define streq(a, b) (strcmp(a, b) == 0)

#define invoke_with_varargs(return_location, function_ptr, ...) do {\
    va_list VA_LIST;\
    va_start(VA_LIST);\
    *return_location = function_ptr(__VA_ARGS__);\
    va_end(VA_LIST);\
} while(0)

#define invoke_with_varargs_void(function_ptr, ...) do {\
    va_list VA_LIST;\
    va_start(VA_LIST);\
    function_ptr(__VA_ARGS__);\
    va_end(VA_LIST);\
} while(0)

/**
 * Printing functions.
 */
int println(void);
int vprintfln(const char *fmt, va_list vlist);
int printfln(const char *fmt, ...);
int eprintln(void);
int veprintf(const char *fmt, va_list vlist);
int veprintfln(const char *fmt, va_list vlist);
int eprintf(const char *fmt, ...);
int eprintfln(const char *fmt, ...);
    
[[noreturn]] void bail(const char *fmt, ...);
char *cause(const char *fmt, ...);
void status(const char *status, const char *fmt, ...);

/**
 * Null-terminated dynamic array of null-terminated strings.
 */
typedef struct {
    char **data;
} Strs;

Strs strs_new();
Strs strs_from_array(char **arr, int length);
Strs strs_clone(Strs *arr);
Strs strs_from_range(Strs *arr, int start, int end);
void strs_free(Strs *arr);
int strs_len(const Strs *arr);
char* strs_push(Strs *arr, const char *str);
char* strs_set(Strs *arr, size_t i, const char *str);
bool strs_contains(const Strs *arr, const char *str);

/**
 * Path manipulation and filesystem functions.
 */
const char *fs_cwd(void);
const char* fs_search_path(const char* name);
bool fs_exists(const char* path);
bool fs_is_regular(const char *path);
bool fs_create_directory_recursive(const char *path);
bool path_is_absolute(const char *path);
char *path_join(const char* a, const char* b);
char *path_get_basename(const char *path);
char *path_get_relative(const char *base, const char *path);
char *path_change_extension(const char *path, const char *extension);

/**
 * Builder for subprocesses. Contains the path to the executable and the
 * arguments to pass to it.
 */
typedef struct ProcessBuilder {
    const char* _path;
    const char* _name;
    Strs args;
} ProcessBuilder;

ProcessBuilder process_new(const char *path);
void process_free(ProcessBuilder* pb);
void process_set_path(ProcessBuilder* pb, const char* path);
void process_set_name(ProcessBuilder* pb, const char* name);
void process_add_arg(ProcessBuilder* pb, const char* arg);

int process_start(const ProcessBuilder* pb);