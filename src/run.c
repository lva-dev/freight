#include "workspace.h"

#include "pch.h"
#include "workspace.h"
#include "util.h"
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

static Strs get_source_files(const WorkspaceContext *ctx) {
    Strs files = strs_new();
    
    DIR* stream = opendir(ws_source_dir(ctx));
    struct dirent* entry;
    while((entry = readdir(stream)) != nullptr) {
        const char *file_name = entry->d_name;
        if (streq(file_name, ".") || streq(file_name, "..")) {
            continue;
        }
        
        char *file_absolute_path = path_join(ws_source_dir(ctx), file_name);
        if (!fs_is_regular(file_absolute_path)) {
            strs_free(&files);
            closedir(stream);
            bail("source file `%s` is not a regular file", file_name);
        }

        strs_push(&files, file_absolute_path);
    }
    
    closedir(stream);
    return files;
}

typedef struct CompilerOptions {
    bool debugging;
} CompilerOptions;

typedef struct Compilation {
    const WorkspaceContext* _ctx;
    CompilerOptions _opts;
    Strs _object_files;
    char *_exe_path;
    bool _had_error;
} Compilation;

static Compilation compilation_new(const WorkspaceContext* ctx, const CompilerOptions* opts) {
    return (Compilation){
        ._ctx = ctx,
        ._opts = *opts,
        ._object_files = strs_new(),
        ._exe_path = path_join(ws_target_dir(ctx), ws_name(ctx)),
        ._had_error = false,
    };
}

static void compilation_free(Compilation *c) {
    strs_free(&c->_object_files);
    free(c->_exe_path);
}

static const char *compilation_exe(Compilation *c) {
    return c->_exe_path;
}

static bool compile_single_file(Compilation* c, const char *source_file) {
    ProcessBuilder pb = process_new(ws_compiler_path(c->_ctx));
    process_add_arg(&pb, source_file);
    process_add_arg(&pb, "-c");
    
    char *src_file_relative = path_get_relative(ws_source_dir(c->_ctx), source_file);
    char *src_file_relative_adjusted = path_change_extension(src_file_relative, "o");
    char *object_file = path_join(ws_object_dir(c->_ctx), src_file_relative_adjusted);
    free(src_file_relative);
    
    process_add_arg(&pb, "-o");
    process_add_arg(&pb, object_file);
    
    if (c->_opts.debugging) {
        process_add_arg(&pb, "-g");
    }
    
    int result = process_start(&pb);
    process_free(&pb);
    
    if (result != 0) {
        c->_had_error = true;
        free(object_file);
        return false;
    }

    strs_push(&c->_object_files, object_file);
    free(object_file);
    return true;
}

static bool link_files(Compilation* c) {
    assert(!c->_had_error);
    
    ProcessBuilder pb = process_new(ws_compiler_path(c->_ctx));
    char *compiler_basename = path_get_basename(ws_compiler_path(c->_ctx));
    process_set_name(&pb, compiler_basename);
    free(compiler_basename);
    
    for (int i = 0; i < strs_len(&c->_object_files); i++) {
        process_add_arg(&pb, c->_object_files.data[i]);    
    }

    process_add_arg(&pb, "-o");
    process_add_arg(&pb, c->_exe_path);
    
    int result = process_start(&pb);
    if (result != 0) {
        c->_had_error = true;
        return false;
    }

    return true;
}

char *compile(Compilation* c) {
    fs_create_directory_recursive(ws_target_dir(c->_ctx));
    fs_create_directory_recursive(ws_object_dir(c->_ctx));
    
    Strs source_files = get_source_files(c->_ctx);
    
    bool found_main_c = false;
    for (int i = 0; i < strs_len(&source_files); i++) {
        char *base = path_get_basename(source_files.data[i]);
        found_main_c = found_main_c || streq(base, "main.c");
        free(base);

        if (found_main_c) {
            break;
        }
    }

    if (!found_main_c) {
        bail("could not find a source file with the name `main.c`");
    }
    
    for (int i = 0; i < strs_len(&source_files); i++) {
        compile_single_file(c, source_files.data[i]);    
    }

    strs_free(&source_files);
    
    if (c->_had_error) {
        bail("could not compile `%s` due to error(s)", ws_name(c->_ctx));
    }
    
    if (!link_files(c)) {
        bail("could not compile `%s` due to linker error(s)", ws_name(c->_ctx));
    }

    return strdup(c->_exe_path);
}

bool exec_build(WorkspaceContext *ctx, char **args) {
    CompilerOptions opts = {
        .debugging = true
    };
    
    Compilation compilation = compilation_new(ctx, &opts);
    compile(&compilation);
    compilation_free(&compilation);
    return true;
}

static double timespec_to_double(struct timespec *ts) {
    return ts->tv_sec + ((double)ts->tv_nsec / 1000000000);
}

static double get_current_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec + ((double)ts.tv_nsec / 1000000000);
}

void exec_run(Strs *args) {
    WorkspaceContext ctx = workspace_open(fs_cwd());
    
    CompilerOptions opts = {
        .debugging = true
    };
    
    status("Compiling", "%s (%s)", ws_name(&ctx), ws_cwd(&ctx));
    
    double comp_start_time = get_current_time();

    Compilation compilation = compilation_new(&ctx, &opts);
    char *exe_path = compile(&compilation);
    compilation_free(&compilation);

    double comp_end_time = get_current_time();
    double time = comp_end_time - comp_start_time;

    status(" Finished", "target(s) in %.2lfs", time);

    char *exe_path_relative = path_get_relative(ws_cwd(&ctx), exe_path);
    status("  Running", "`%s`", exe_path_relative);
    free(exe_path_relative);
    
    ProcessBuilder pb = process_new(exe_path);
    free(exe_path);
    exit(process_start(&pb));
}