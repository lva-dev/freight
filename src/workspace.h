#pragma once

typedef struct WorkspaceContext {
    char *_cwd;
    char *_name;
    char *_target_dir;
    char *_object_dir;
    char *_source_dir;
    char *_compiler_path;
} WorkspaceContext;

bool workspace_exists(const char *cwd);
WorkspaceContext workspace_create(const char* path);
WorkspaceContext workspace_open(const char* cwd);
void workspace_free(const WorkspaceContext* workspace);

const char* ws_cwd(const WorkspaceContext* ws);
const char* ws_name(const WorkspaceContext* ws);
const char* ws_target_dir(const WorkspaceContext* ws);
const char* ws_object_dir(const WorkspaceContext* ws);
const char* ws_source_dir(const WorkspaceContext* ws);
const char* ws_compiler_path(const WorkspaceContext* ws);