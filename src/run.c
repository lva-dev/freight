#include "workspace.h"

#include "pch.h"
#include "workspace.h"
#include "util.h"
#include <dirent.h>
#include <stdlib.h>
#include <sys/stat.h>

static Strs get_source_files(const WorkspaceContext *ctx) {
    Strs files = strs_new();
    
    DIR* stream = opendir(ctx->_source_dir);
    struct dirent* entry;
    while((entry = readdir(stream))) {
        const char *filename = entry->d_name;
        if (!fs_is_regular(filename)) {
            strs_free(files);
            bail("source file `%s` is not a regular file", filename);
            return NULL;
        } 

        strs_push(files, entry->d_name);    
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
    bool _had_error;
} Compilation;

static Compilation compilation_new(const WorkspaceContext* ctx, const CompilerOptions* opts) {
    return (Compilation){
        ._ctx = ctx,
        ._opts = *opts,
        ._object_files = NULL,
        ._had_error = false,
    };
}

static void compilation_free(Compilation *c) {
    strs_free(c->_object_files);
}

static bool compile_single_file(Compilation* c, const char *source_file) {
    ProcessBuilder pb = process_new(ws_compiler_path(c->_ctx));
    process_add_arg(&pb, source_file);
    
    char *source_file_relative = path_get_relative(ws_source_dir(c->_ctx), source_file);
    const char *object_file = path_join(ws_object_dir(c->_ctx), source_file_relative);
    process_add_arg(&pb, "-o");
    process_add_arg(&pb, object_file);

    if (c->_opts.debugging) {
        process_add_arg(&pb, strdup("-g"));
    }

    int result = process_start(&pb);
    if (result != 0) {
        c->_had_error = true;
        return false;
    }

    return true;
}

static bool link_files(Compilation* c) {
    assert(!c->_had_error);
    
    ProcessBuilder pb = process_new(ws_compiler_path(c->_ctx));

    for (int i = 0; i < strs_len(c->_object_files); i++) {
        process_add_arg(&pb, c->_object_files[i]);    
    }

    int result = process_start(&pb);
    if (result != 0) {
        c->_had_error = true;
        return false;
    }

    return true;
}

void compile(Compilation* c) {
    status("Compiling", "");

    fs_create_directory_recursive(ws_target_dir(c->_ctx));
    fs_create_directory_recursive(ws_object_dir(c->_ctx));
    
    Strs source_files = get_source_files(c->_ctx);
    
    bool found_main_c = false;
    for (int i = 0; i < strs_len(source_files); i++) {
        char *basename = path_get_basename(source_files[i]);
        if (streq(basename, "main.c")) {
            found_main_c = true;
            free(basename);
            break;
        }
    }

    if (!found_main_c) {
        bail("could not find a source file with the name `main.c`");
    }
    
    for (int i = 0; i < strs_len(source_files); i++) {
        compile_single_file(c, source_files[i]);    
    }

    strs_free(source_files);
    
    if (c->_had_error) {
        bail("could not compile `%s` due to error(s)", ws_name(c->_ctx));
    }
    
    if (!link_files(c)) {
        bail("could not compile `%s` due to linker error(s)", ws_name(c->_ctx));
    }
}

bool exec_build(WorkspaceContext *ctx, char **args) {
    CompilerOptions opts = {
        .debugging = true
    };
    
    Compilation compilation = compilation_new(ctx, &opts);
    compilation_free(&compilation);
    return true;
}

void exec_run(WorkspaceContext* ctx, char **args) {
    CompilerOptions opts = {
        .debugging = true
    };
    
    Compilation compilation = compilation_new(ctx, &opts);
    compile(&compilation);
    compilation_free(&compilation);
}