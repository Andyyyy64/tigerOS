#ifndef PATH_STATE_H
#define PATH_STATE_H

#include <stddef.h>

#include "fs_dir.h"

typedef struct {
  fs_dir_tree_t dir_tree;
  int ready;
} path_state_context_t;

typedef enum {
  PATH_STATE_ENTRY_DIR = 1,
  PATH_STATE_ENTRY_FILE = 2,
} path_state_entry_kind_t;

typedef struct {
  char name[FS_DIR_NAME_MAX + 1u];
  path_state_entry_kind_t kind;
} path_state_entry_t;

void path_state_context_init(path_state_context_t *context);
int path_state_pwd_ctx(path_state_context_t *context, char *out, size_t out_len);
int path_state_cd_ctx(path_state_context_t *context, const char *path);
int path_state_mkdir_ctx(path_state_context_t *context, const char *path);
int path_state_ls_ctx(path_state_context_t *context,
                      const char *path,
                      path_state_entry_t *entries,
                      size_t max_entries,
                      size_t *out_count);
int path_state_cat_ctx(path_state_context_t *context, const char *path, const char **out_contents);

void path_state_init(void);
int path_state_pwd(char *out, size_t out_len);
int path_state_cd(const char *path);
int path_state_mkdir(const char *path);
int path_state_ls(const char *path,
                  path_state_entry_t *entries,
                  size_t max_entries,
                  size_t *out_count);
int path_state_cat(const char *path, const char **out_contents);

#endif
