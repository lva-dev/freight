#include "workspace.h"
#include "util.h"
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

bool workspace_exists(const char *cwd) {
    char *stim_toml = path_join(cwd, "Stim.toml");
    if (fs_exists(stim_toml)) {
        free(stim_toml);
        return true;
    }

    free(stim_toml);
    return false;
}

bool create_manifest_file(WorkspaceContext* ctx) {
    char *path = path_join(ws_cwd(ctx), "Stim.toml");
    FILE* file = fopen(path, "w");
    free(path);
    
    if (!file) {
        return false; 
    }
    
    static const char *fmt = "[package]\n"
        "name = \"%s\"\n"
        "version = \"0.1.0\""
        "standard = \"c23\"";

    fprintf(file, fmt, ws_name(ctx));
    fclose(file);
    return true;
}

bool create_main_c_file(const char *source_dir) {
    char *path = path_join(source_dir, "main.c");
    FILE* file = fopen(path, "w");
    free(path);

    if (!file) {
        return false; 
    }
    
    static const char *SOURCE =
        "#include <stdio.h>\n"
        "\n"
        "int main(int argc, char **argv) {\n"
        "    printf(\"Hello, World!\\n\");    \n"
        "}\n";

    fputs(SOURCE, file);
    fclose(file);
    return true;
}

WorkspaceContext workspace_create(const char* path) {
    // if (workspace_exists(path)) {
    //     bail("a workspace already exists in this directory");
    // }
    
    char *name = path_get_basename(path);
    if (streq(name, "")) {
        bail("project name cannot be empty");
    }
    
    WorkspaceContext ws = {
        ._cwd = strdup(path),
        ._name = name,
        ._target_dir = NULL,
        ._object_dir = NULL,
        ._source_dir = NULL,
    };
       
    if (!fs_create_directory_recursive(ws_source_dir(&ws))) {
        bail("failed to create project\n\n%s", cause(strerror(errno)));
    }
    
    create_manifest_file(&ws);
    create_main_c_file(ws_source_dir(&ws));
    return ws;
}

WorkspaceContext workspace_open(const char* cwd) {
    if (!workspace_exists(cwd)) {
        bail("could not find a workspace in this directory");
    }

    WorkspaceContext ws;
    ws._cwd = strdup(cwd);
    ws._name = path_get_basename(cwd);
    ws._target_dir = path_join(cwd, "target");
    ws._object_dir = strdup(ws._target_dir);
    ws._source_dir = path_join(cwd, "src");
    ws._compiler_path = NULL;
    return ws;
}

const char* ws_cwd(const WorkspaceContext* ws) {
    assert(ws->_name != NULL);
    return ws->_cwd;
}

const char* ws_name(const WorkspaceContext* ws) {
    assert(ws->_name != NULL);
    return ws->_name;
}

const char* ws_target_dir(const WorkspaceContext* ws) {
    if (ws->_target_dir != NULL) {
        return ws->_target_dir;
    }

    static const char *target_dir = NULL;
    if (!target_dir) {
        target_dir = path_join(ws_cwd(ws), "target");
    }
    
    return target_dir;
}

const char* ws_object_dir(const WorkspaceContext* ws) {
    if (ws->_object_dir != NULL) {
        return ws->_object_dir;
    }

    return ws_target_dir(ws);
}

const char* ws_source_dir(const WorkspaceContext* ws) {
    if (ws->_source_dir != NULL) {
        return ws->_source_dir;
    }

    static const char *source_dir = NULL;
    if (!source_dir) {
        source_dir = path_join(ws_cwd(ws), "src");
    }
    
    return source_dir;
}

const char* ws_compiler_path(const WorkspaceContext* ws) {
    if (ws->_compiler_path != NULL) {
        return ws->_compiler_path;
    }

    static const char *compiler_path = NULL;
    if (!compiler_path) {
        compiler_path = fs_search_path("clang");
    }
    
    return compiler_path;
}

void workspace_free(const WorkspaceContext* ws) {
    free(ws->_cwd);
    free(ws->_name);
    free(ws->_target_dir);
    free(ws->_object_dir);
    free(ws->_source_dir);
}