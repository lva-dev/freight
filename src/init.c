#include "pch.h"
#include "util.h"
#include "workspace.h"
#include "cmds.h"
#include <string.h>
#include <sys/stat.h>

void exec_new(Strs *args) {
    if (strs_len(args) < 1) {
        bail("missing required argument: <PATH>");
    }
    
    const char *path_arg = args->data[0];
    char *project_path = path_is_absolute(path_arg)
        ? strdup(path_arg) : path_join(fs_cwd(), path_arg);

    status("Creating", "binary `%s` project", path_arg);

    if (fs_exists(project_path)) {
        bail("directory `%s` already exists");
    }

    workspace_create(project_path);
    
    free(project_path);
}