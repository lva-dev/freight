#include "cwalk.h"
#include "pch.h"
#include <errno.h>
#include <limits.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
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
    printf("   \033[m%s\033[m ", status);
    invoke_with_varargs_void(vprintfln, fmt, VA_LIST);
    exit(1);
}

Strs strs_new() {
    Strs new = NULL;
    arrpush(new, NULL);
    return new;
}

Strs strs_from_array(char** array, int length) {
    Strs new = NULL;
    
    for (int i = 0; i < length; i++) {
        arrpush(new, strdup(array[i]));
    }
    
    arrpush(new, NULL);
    return new;
}

void strs_free(Strs array) {
    int len = strs_len(array);
    for (int i = 0; i < len; i++) {
        free(array[i]);
    }

    arrfree(array);
}

Strs strs_clone(Strs array) {
    Strs new = NULL;
    
    int len = arrlen(array);
    arrsetlen(new, len);
    for (int i = 0; i < len; i++) {
        new[i] = strdup(array[i]);
    }

    return new;
}

Strs strs_subrange(Strs array, int start, int end) {
    Strs new_strs = NULL;
    
    int end_index = (end == -1) ? strs_len(array) : end;
    for (int i = start; i < end_index; i++) {
        arrpush(new_strs, array[i]);
    }

    arrpush(new_strs, NULL);
    return new_strs;
}

int strs_len(Strs array) {
    return arrlen(array) - 1;
}

char* strs_push(Strs array, const char *str) {
    return arrins(array, strs_len(array), strdup(str));
}

bool strs_contains(Strs array, const char *str) {
    int len = strs_len(array);
    for (int i = 0; i < len; i++) {
        if (streq(array[i], str)) {
            return true;
        }
    }

    return false;
}


const char *fs_cwd(void) {
    static const char *cwd = NULL;
    if (cwd == NULL) {
        cwd = get_current_dir_name();
    }
    
    return cwd;
}

const char* fs_search_path(const char* name) {
    const char *path_variable = getenv("PATH");
    if (!path_variable) {
        bail("");
    }

    char *path = strdup(path_variable);
    const char *directory;
    char joined_path[FILENAME_MAX];
    
    while ((directory = strtok(path, ":"))) {
        cwk_path_join(directory, name, joined_path, FILENAME_MAX);
        if (fs_exists(joined_path)) {
            free(path);
            return strdup(joined_path);
        }
    }
    
    free(path);
    return NULL;
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
    unreachable();
}

bool fs_is_regular(const char *path) {
    if (!fs_exists(path)) {
        return false;
    }

    struct stat sb;
    int result = stat(path, &sb);
    if (result == -1) {
        bail("failed to get file status\n\n%s", cause(strerror(errno)));
    }

    return S_ISREG(sb.st_mode);
}

bool fs_is_directory(const char *path) {
    if (!fs_exists(path)) {
        return false;
    }

    struct stat sb;
    int result = stat(path, &sb);
    if (result == - 1) {
        bail("failed to get file status\n\n%s", cause(strerror(errno)));
    }

    return S_ISDIR(sb.st_mode);
}

bool fs_is_link(const char *path) {
    if (!fs_exists(path)) {
        return false;
    }

    struct stat sb;
    int result = stat(path, &sb);
    if (result == -1) {
        bail("failed to get file status\n\n%s", cause(strerror(errno)));
    }

    return S_ISLNK(sb.st_mode);
}

typedef struct {
    char *_buffer;
} PathBuf;

static void pb_free(PathBuf* buf) {
    arrfree(buf->_buffer);
}

static int pb_append(PathBuf* buf, struct cwk_segment *segment) {
    arrput(buf->_buffer, '/');
    for (size_t i = 0; i < segment->size; i++) {
        arrput(buf->_buffer, segment->begin[i]);
    }

    return segment->size + 1;
}

static const char *pb_get(PathBuf* buf) {
    return buf->_buffer;
}

static bool is_last_segment(struct cwk_segment* segment, const char *path) {
    struct cwk_segment last_segment;
    cwk_path_get_last_segment(path, &last_segment);

    if (segment->begin == last_segment.begin && segment->end == last_segment.end) {
        return true;
    }

    return false;
}

bool fs_create_directory_recursive(const char *path) {
    struct cwk_segment segment;
    bool has_segment = cwk_path_get_first_segment(path, &segment);
    if (!has_segment) {
        return false;
    }

    static const mode_t UMASK = 0777;
    bool directory_made = false;
    
    PathBuf current_path;
    do {
        printf("Segment: '%.*s'.\n", (int)segment.size, segment.begin);
        pb_append(&current_path, &segment);
        int mkdir_result = mkdir(pb_get(&current_path), UMASK);
        if (mkdir_result == 0) {
            directory_made = true;
            continue;  
        }

        bool good_error = errno == EEXIST && fs_is_directory(pb_get(&current_path));
        if (!good_error) {
            bail("failed to create directory\n\n%s", cause("%s", strerror(errno)));
        }
    } while (cwk_path_get_next_segment(&segment));

    pb_free(&current_path);
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
    return (ProcessBuilder){
        .path = strdup(path),
        .args = strs_new(),
    };
}

void process_free(ProcessBuilder* pb) {
    strs_free(pb->args);
}

void process_set_path(ProcessBuilder* pb, const char* path) {
    pb->path = strdup(path);
}

void process_add_arg(ProcessBuilder* pb, const char* arg) {
    strs_push(pb->args, arg);
}

int process_start(const ProcessBuilder* pb) {
    assert(path_is_absolute(pb->path) && fs_exists(pb->path));
    
    pid_t pid = fork();
    
    if (pid == -1) {
        abort();
    }
    
    if (pid == 0) {
        int status;
        waitpid(pid, &status, 1);
        return WEXITSTATUS(status);
    }

    execv(pb->path, pb->args);
    bail("system error\n\n%s", cause("%s", strerror(errno)));
}