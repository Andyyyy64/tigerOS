#ifndef FS_DIR_H
#define FS_DIR_H

#include <stddef.h>
#include <stdint.h>

#include "fs_path.h"

#define FS_DIR_NAME_MAX 31u
#define FS_DIR_MAX_NODES 128u

typedef struct {
  char name[FS_DIR_NAME_MAX + 1u];
} fs_dirent_t;

typedef struct {
  char name[FS_DIR_NAME_MAX + 1u];
  int parent;
  int first_child;
  int next_sibling;
  uint8_t used;
} fs_dir_node_t;

typedef struct {
  fs_dir_node_t nodes[FS_DIR_MAX_NODES];
  uint16_t node_count;
  int cwd_index;
} fs_dir_tree_t;

void fs_dir_init(fs_dir_tree_t *tree);
int fs_dir_walk(const fs_dir_tree_t *tree, const char *path, int *out_index);
int fs_dir_mkdir(fs_dir_tree_t *tree, const char *path);
int fs_dir_mkdir_p(fs_dir_tree_t *tree, const char *path);
int fs_dir_readdir(const fs_dir_tree_t *tree,
                   const char *path,
                   fs_dirent_t *entries,
                   size_t max_entries,
                   size_t *out_count);
int fs_dir_cd(fs_dir_tree_t *tree, const char *path);
int fs_dir_pwd(const fs_dir_tree_t *tree, char *out, size_t out_len);

#endif
