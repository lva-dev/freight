#include "cwalk.h"
#include "pch.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "util.h"

int println(void) {
    int result = fputc('\n', stdout);
    if (result != EOF) {
        return 1;
    } else {
        return -1;
    }
}

int vprintfln(const char *fmt, va_list vlist) {
    int bytes_written = vprintf(fmt, vlist);
    if (bytes_written == -1) {
        return -1;
    }

    int newlines_written = println();
    if (newlines_written == -1) {
        return -1;
    }
    
    static constexpr const int NUM_NEWLINES = 1;
    return bytes_written + NUM_NEWLINES;
}

int printfln(const char *fmt, ...) {
    int bytes_written;
    invoke_with_varargs(&bytes_written, vprintfln, fmt, VA_LIST);
    return bytes_written;    
}

int eprintln(void) {
    int result = fputc('\n', stderr);
    if (result != EOF) {
        return 1;
    } else {
        return -1;
    }
}

int veprintf(const char *fmt, va_list vlist) {
    return vfprintf(stderr, fmt, vlist);
}

int veprintfln(const char *fmt, va_list vlist) {
    int bytes_written = veprintf(fmt, vlist);
    if (bytes_written == -1) {
        return -1;
    }

    int newlines_written = eprintln();
    if (newlines_written == -1) {
        return -1;
    }
    
    static constexpr const int NUM_NEWLINES = 1;
    return bytes_written + NUM_NEWLINES;
}

int eprintf(const char *fmt, ...) {
    int bytes_written;
    invoke_with_varargs(&bytes_written, veprintf, fmt, VA_LIST);
    return bytes_written;
}

int eprintfln(const char *fmt, ...) {
    int bytes_written;
    invoke_with_varargs(&bytes_written, veprintfln, fmt, VA_LIST);
    return bytes_written;  
}


[[noreturn]] void bail(const char *fmt, ...) {
    eprintf("\033[31merror:\033[39m ");
    invoke_with_varargs_void(veprintfln, fmt, VA_LIST);
    exit(1);
}

char *cause(const char *fmt, ...) {
    static constexpr char PREFIX[] = "Cause: ";
    static constexpr int PREFIX_LEN = sizeof(PREFIX) - 1;
    static constexpr int BUF_SIZE = 512;

    char *buf = malloc(BUF_SIZE);
    strcpy(buf, PREFIX);
    
    invoke_with_varargs_void(vsnprintf, buf + PREFIX_LEN, BUF_SIZE, fmt, VA_LIST);
    return buf;
}

void status(const char *status, const char *fmt, ...) {
    printf("   \033[32m%s\033[m ", status);
    invoke_with_varargs_void(vprintfln, fmt, VA_LIST);
}

Strs strs_new() {
    char **data = nullptr;
    arrpush(data, nullptr);
    return (Strs){.data = data};
}

Strs strs_clone(Strs *arr) {
    return strs_from_range(arr, 0, -1);
}

Strs strs_from_array(char** arr, int length) {
    char **data = nullptr;
    
    for (int i = 0; i < length; i++) {
        arrpush(data, strdup(arr[i]));
    }
    
    arrpush(data, nullptr);
    return (Strs){.data = data};;
}

Strs strs_from_range(Strs *arr, int start, int end) {
    char** data = nullptr;    
    int end_index = (end == -1) ? strs_len(arr) : end;
    for (int i = start; i < end_index; i++) {
        arrpush(data, strdup(arr->data[i]));
    }

    arrpush(data, nullptr);
    return (Strs){.data = data};
}

void strs_free(Strs *arr) {
    for (int i = 0; i < strs_len(arr); i++) {
        free(arr->data[i]);
    }

    arrfree(arr->data);
}

int strs_len(const Strs *arr) {
    return arrlen(arr->data) - 1;
}

char* strs_push(Strs *arr, const char *str) {
    assert(arr->data[arrlen(arr->data) - 1] == nullptr && "Last element of string array is not a null pointer");
    return arrins(arr->data, strs_len(arr) - 1, strdup(str));
}

char* strs_set(Strs *arr, size_t i, const char *str) {
    free(arr->data[i]);
    return arr->data[i] = strdup(str);
}

bool strs_contains(const Strs *arr, const char *str) {
    int len = strs_len(arr);
    for (int i = 0; i < len; i++) {
        if (streq(arr->data[i], str)) {
            return true;
        }
    }

    return false;
}

const char *fs_cwd(void) {
    static const char *cwd = nullptr;
    if (cwd == nullptr) {
        cwd = get_current_dir_name();
    }
    
    return cwd;
}

const char* fs_search_path(const char* name) {
    const char *path_variable = getenv("PATH");
    if (!path_variable) {
        return nullptr;
    }

    char *path = strdup(path_variable);
    char joined_path[FILENAME_MAX];
    char *dir_token = strtok(path, ":");
    while (dir_token) {
        cwk_path_join(dir_token, name, joined_path, FILENAME_MAX);
        if (fs_exists(joined_path)) {
            free(path);
            return strdup(joined_path);
        }

        dir_token = strtok(nullptr, ":");
    }
    
    free(path);
    return nullptr;
}

bool fs_exists(const char* path) {
    struct stat sb;
    int result = stat(path, &sb);
    if (result == 0) {
        return true;
    }

    if (errno == ENOENT) {
        return false;
    }

    bail("failed to get file status\n\n%s", cause(strerror(errno)));
}

static struct stat get_file_status_or_bail(const char *path) {
    struct stat sb;
    int result = stat(path, &sb);
    if (result == -1) {
        bail("failed to get file status\n\n%s", cause(strerror(errno)));
    }

    return sb;
}

bool fs_is_regular(const char *path) {
    if (!fs_exists(path)) {
        return false;
    }

    struct stat sb = get_file_status_or_bail(path);
    return S_ISREG(sb.st_mode);
}

bool fs_is_directory(const char *path) {
    if (!fs_exists(path)) {
        return false;
    }

    struct stat sb = get_file_status_or_bail(path);
    return S_ISDIR(sb.st_mode);
}

bool fs_is_link(const char *path) {
    if (!fs_exists(path)) {
        return false;
    }

    struct stat sb = get_file_status_or_bail(path);
    return S_ISLNK(sb.st_mode);
}

typedef struct {
    char *_buffer;
} PathBuf;

static PathBuf pathbuf_new() {
    PathBuf pathbuf = {
        ._buffer = nullptr,
    };

    arrput(pathbuf._buffer, '\0');
    return pathbuf;
}

static void pathbuf_free(PathBuf* buf) {
    arrfree(buf->_buffer);
}

static int pb_append(PathBuf* buf, struct cwk_segment *segment) {
    arrpop(buf->_buffer);
    arrput(buf->_buffer, '/');
    for (size_t i = 0; i < segment->size; i++) {
        arrput(buf->_buffer, segment->begin[i]);
    }

    arrput(buf->_buffer, '\0');

    return segment->size + 1;
}

static const char *pathbuf_get(PathBuf* buf) {
    return buf->_buffer;
}

[[maybe_unused]] static bool is_last_segment(struct cwk_segment* segment, const char *path) {
    struct cwk_segment last_segment;
    cwk_path_get_last_segment(path, &last_segment);

    if (segment->begin == last_segment.begin && segment->end == last_segment.end) {
        return true;
    }

    return false;
}

bool fs_create_directory_recursive(const char *path) {
    struct cwk_segment segment;
    if (!cwk_path_get_first_segment(path, &segment)) {
        return false;
    }

    static const mode_t UMASK = 0777;
    bool directory_made = false;
    
    PathBuf current_path = pathbuf_new();
    do {
        pb_append(&current_path, &segment);
        if (mkdir(pathbuf_get(&current_path), UMASK) == 0) {
            directory_made = true;
        } else {
            // There should never be a "file does not exist" error from this
            // function, unless from a dangling symlink
            assert(errno != ENOENT || fs_is_link(pathbuf_get(&current_path)));

            bool good_error = errno == EEXIST && fs_is_directory(pathbuf_get(&current_path));
            if (!good_error) {
                bail("failed to create directory\n\n%s", cause("%s", strerror(errno)));
            }
        }
    } while (cwk_path_get_next_segment(&segment));

    pathbuf_free(&current_path);
    return directory_made;
}

bool path_is_absolute(const char *path) {
    return cwk_path_is_absolute(path);
}

char* path_join(const char* a, const char* b) {
    size_t buf_size = strlen(a) + strlen(b) + 2;
    char* buf = malloc(buf_size);
    cwk_path_join(a, b, buf, buf_size);
    return buf;
}

char *path_get_basename(const char *path) {
    const char *basename_start;
    size_t size;
    cwk_path_get_basename(path, &basename_start, &size);
    char* buf = malloc(size + 1);
    memcpy(buf, basename_start, size);
    buf[size] = '\0';
    return buf;
}

char *path_get_basename_no_ext(const char *path) {
    struct cwk_segment seg;
    cwk_path_get_last_segment(path, &seg);    
    const char *dot = strchr(seg.begin, '.');
    if (dot) {
        size_t size = dot - seg.begin;
        char *buf = malloc(size + 1);
        memcpy(buf, seg.begin, size);
        buf[size] = '\0';
        return buf;
    } else {
        return strdup(seg.begin);
    }
}

char *path_get_relative(const char *base, const char *path) {
    size_t buf_size = strlen(path);
    char* buf = malloc(buf_size);
    cwk_path_get_relative(base, path, buf, buf_size);
    return buf;
}

char *path_change_extension(const char *path, const char *extension) {
    size_t buf_size = strlen(path) + strlen(extension) + 1;
    char* buf = malloc(buf_size);
    cwk_path_change_extension(path, extension, buf, buf_size);
    return buf;
}

ProcessBuilder process_new(const char *path) {
    Strs args = strs_new();
    char *name = path_get_basename(path);
    strs_push(&args, name);
    free(name);
    return (ProcessBuilder){
        ._path = strdup(path),
        .args = args,
    };
}

void process_free(ProcessBuilder* pb) {
    strs_free(&pb->args);
}

void process_set_path(ProcessBuilder* pb, const char* path) {
    pb->_path = strdup(path);
}

void process_set_name(ProcessBuilder* pb, const char* name) {
    strs_set(&pb->args, 0, name);
}

void process_add_arg(ProcessBuilder* pb, const char* arg) {
    strs_push(&pb->args, arg);
}

static int *create_shared_int(int value) {
    int *object = mmap(nullptr, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    *object = value;
    return object;
}

static bool destroy_shared_int(int* b) {
    int result = munmap(b, sizeof(int));
    return result == 0;
}

int process_start(const ProcessBuilder* pb) {
    assert(path_is_absolute(pb->_path) && fs_exists(pb->_path));
    
    int *error_number = create_shared_int(0);
    if (error_number == nullptr) {
        bail("Failed to start child process\n\n%s", cause("%s", strerror(errno)));
    }

    pid_t pid = fork();
    
    if (pid == -1) {
        *error_number = errno;
    }
    
    if (pid == 0) {
        execv(pb->_path, pb->args.data);        
        *error_number = errno;
        abort();
    }

    int status;
    waitpid(pid, &status, 0);
    
    if (*error_number != 0) {
        int err = *error_number;
        destroy_shared_int(error_number);
        bail("Failed to start child process\n\n%s", cause("%s", strerror(err)));
    }

    destroy_shared_int(error_number);
    return WEXITSTATUS(status);
}