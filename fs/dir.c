#include <stddef.h>
#include <string.h>

#include "fs_dir.h"

static int valid_tree(const fs_dir_tree_t *tree) {
  if (tree == NULL || tree->node_count == 0u || tree->node_count > FS_DIR_MAX_NODES ||
      tree->cwd_index < 0 || tree->cwd_index >= (int)tree->node_count ||
      tree->nodes[tree->cwd_index].used == 0u || tree->nodes[0].used == 0u) {
    return 0;
  }
  return 1;
}

static int find_child(const fs_dir_tree_t *tree, int parent_index, const char *name) {
  int cur = tree->nodes[parent_index].first_child;
  while (cur >= 0) {
    if (tree->nodes[cur].used != 0u && strcmp(tree->nodes[cur].name, name) == 0) {
      return cur;
    }
    cur = tree->nodes[cur].next_sibling;
  }
  return -1;
}

static int alloc_node(fs_dir_tree_t *tree) {
  int idx;
  if (tree->node_count >= FS_DIR_MAX_NODES) {
    return -1;
  }

  idx = (int)tree->node_count;
  memset(&tree->nodes[idx], 0, sizeof(tree->nodes[idx]));
  tree->nodes[idx].used = 1u;
  tree->nodes[idx].parent = -1;
  tree->nodes[idx].first_child = -1;
  tree->nodes[idx].next_sibling = -1;
  tree->node_count++;
  return idx;
}

static int insert_child_sorted(fs_dir_tree_t *tree, int parent_index, int child_index) {
  int prev = -1;
  int cur = tree->nodes[parent_index].first_child;

  while (cur >= 0 && strcmp(tree->nodes[cur].name, tree->nodes[child_index].name) < 0) {
    prev = cur;
    cur = tree->nodes[cur].next_sibling;
  }

  if (prev < 0) {
    tree->nodes[child_index].next_sibling = tree->nodes[parent_index].first_child;
    tree->nodes[parent_index].first_child = child_index;
  } else {
    tree->nodes[child_index].next_sibling = tree->nodes[prev].next_sibling;
    tree->nodes[prev].next_sibling = child_index;
  }

  return 0;
}

static int path_from_index(const fs_dir_tree_t *tree, int index, char *out, size_t out_len) {
  int stack[FS_DIR_MAX_NODES];
  size_t depth = 0u;
  size_t out_used = 0u;
  int cur = index;

  if (!valid_tree(tree) || out == NULL || out_len == 0u || index < 0 ||
      index >= (int)tree->node_count || tree->nodes[index].used == 0u) {
    return -1;
  }

  if (index == 0) {
    if (out_len < 2u) {
      return -1;
    }
    out[0] = '/';
    out[1] = '\0';
    return 0;
  }

  while (cur > 0) {
    if (depth >= FS_DIR_MAX_NODES) {
      return -1;
    }
    stack[depth++] = cur;
    cur = tree->nodes[cur].parent;
    if (cur < 0 || cur >= (int)tree->node_count || tree->nodes[cur].used == 0u) {
      return -1;
    }
  }

  if (out_len < 2u) {
    return -1;
  }
  out[out_used++] = '/';

  while (depth > 0u) {
    const char *name;
    size_t name_len;

    depth--;
    name = tree->nodes[stack[depth]].name;
    name_len = strlen(name);
    if (name_len == 0u || name_len > FS_DIR_NAME_MAX || out_used + name_len + 1u > out_len) {
      return -1;
    }
    memcpy(&out[out_used], name, name_len);
    out_used += name_len;

    if (depth > 0u) {
      if (out_used + 2u > out_len) {
        return -1;
      }
      out[out_used++] = '/';
    }
  }

  out[out_used] = '\0';
  return 0;
}

static int resolve_path(const fs_dir_tree_t *tree, const char *path, char *resolved, size_t len) {
  char cwd[FS_PATH_MAX];
  if (!valid_tree(tree) || path == NULL || resolved == NULL || len == 0u) {
    return -1;
  }
  if (path_from_index(tree, tree->cwd_index, cwd, sizeof(cwd)) != 0) {
    return -1;
  }
  return fs_path_resolve(cwd, path, resolved, len);
}

static int lookup_absolute(const fs_dir_tree_t *tree, const char *absolute_path) {
  int cur = 0;
  size_t i = 1u;

  if (!valid_tree(tree) || absolute_path == NULL || absolute_path[0] != '/') {
    return -1;
  }
  if (absolute_path[1] == '\0') {
    return 0;
  }

  while (absolute_path[i] != '\0') {
    char name[FS_DIR_NAME_MAX + 1u];
    size_t start = i;
    size_t len = 0u;
    int child;

    while (absolute_path[i] != '\0' && absolute_path[i] != '/') {
      ++i;
    }

    len = i - start;
    if (len == 0u || len > FS_DIR_NAME_MAX) {
      return -1;
    }
    memcpy(name, &absolute_path[start], len);
    name[len] = '\0';

    child = find_child(tree, cur, name);
    if (child < 0) {
      return -1;
    }
    cur = child;

    if (absolute_path[i] == '/') {
      ++i;
    }
  }

  return cur;
}

static int mkdir_internal(fs_dir_tree_t *tree, const char *path, int create_parents) {
  char absolute[FS_PATH_MAX];
  int cur = 0;
  int created_any = 0;
  size_t i = 1u;

  if (!valid_tree(tree) || path == NULL) {
    return -1;
  }
  if (resolve_path(tree, path, absolute, sizeof(absolute)) != 0) {
    return -1;
  }
  if (strcmp(absolute, "/") == 0) {
    return -1;
  }

  while (absolute[i] != '\0') {
    char name[FS_DIR_NAME_MAX + 1u];
    size_t start = i;
    size_t len = 0u;
    int at_last_component = 0;
    int child;

    while (absolute[i] != '\0' && absolute[i] != '/') {
      ++i;
    }
    len = i - start;
    at_last_component = (absolute[i] == '\0');

    if (len == 0u || len > FS_DIR_NAME_MAX) {
      return -1;
    }
    memcpy(name, &absolute[start], len);
    name[len] = '\0';

    child = find_child(tree, cur, name);
    if (child >= 0) {
      if (at_last_component && !create_parents) {
        return -1;
      }
      cur = child;
    } else {
      int new_node;
      int parent = cur;

      if (!create_parents && !at_last_component) {
        return -1;
      }

      new_node = alloc_node(tree);
      if (new_node < 0) {
        return -1;
      }

      memcpy(tree->nodes[new_node].name, name, len + 1u);
      tree->nodes[new_node].parent = parent;
      tree->nodes[new_node].first_child = -1;
      tree->nodes[new_node].next_sibling = -1;
      insert_child_sorted(tree, parent, new_node);
      cur = new_node;
      created_any = 1;
    }

    if (absolute[i] == '/') {
      ++i;
    }
  }

  if (!create_parents && !created_any) {
    return -1;
  }
  return 0;
}

void fs_dir_init(fs_dir_tree_t *tree) {
  if (tree == NULL) {
    return;
  }

  memset(tree, 0, sizeof(*tree));
  tree->node_count = 1u;
  tree->cwd_index = 0;
  tree->nodes[0].used = 1u;
  tree->nodes[0].parent = -1;
  tree->nodes[0].first_child = -1;
  tree->nodes[0].next_sibling = -1;
  tree->nodes[0].name[0] = '\0';
}

int fs_dir_walk(const fs_dir_tree_t *tree, const char *path, int *out_index) {
  char absolute[FS_PATH_MAX];
  int idx;

  if (!valid_tree(tree) || path == NULL || out_index == NULL) {
    return -1;
  }

  if (resolve_path(tree, path, absolute, sizeof(absolute)) != 0) {
    return -1;
  }

  idx = lookup_absolute(tree, absolute);
  if (idx < 0) {
    return -1;
  }

  *out_index = idx;
  return 0;
}

int fs_dir_mkdir(fs_dir_tree_t *tree, const char *path) { return mkdir_internal(tree, path, 0); }

int fs_dir_mkdir_p(fs_dir_tree_t *tree, const char *path) {
  return mkdir_internal(tree, path, 1);
}

int fs_dir_readdir(const fs_dir_tree_t *tree,
                   const char *path,
                   fs_dirent_t *entries,
                   size_t max_entries,
                   size_t *out_count) {
  int dir_idx = -1;
  int cur;
  size_t count = 0u;
  int overflow = 0;

  if (out_count == NULL) {
    return -1;
  }
  *out_count = 0u;

  if (!valid_tree(tree) || path == NULL || (entries == NULL && max_entries != 0u)) {
    return -1;
  }
  if (fs_dir_walk(tree, path, &dir_idx) != 0) {
    return -1;
  }

  cur = tree->nodes[dir_idx].first_child;
  while (cur >= 0) {
    if (tree->nodes[cur].used == 0u) {
      return -1;
    }

    if (entries != NULL && count < max_entries) {
      memcpy(entries[count].name, tree->nodes[cur].name, FS_DIR_NAME_MAX + 1u);
    } else if (entries != NULL) {
      overflow = 1;
    }

    count++;
    cur = tree->nodes[cur].next_sibling;
  }

  *out_count = count;
  if (overflow != 0) {
    return -1;
  }
  return 0;
}

int fs_dir_cd(fs_dir_tree_t *tree, const char *path) {
  int idx = -1;

  if (!valid_tree(tree) || path == NULL) {
    return -1;
  }
  if (fs_dir_walk(tree, path, &idx) != 0) {
    return -1;
  }

  tree->cwd_index = idx;
  return 0;
}

int fs_dir_pwd(const fs_dir_tree_t *tree, char *out, size_t out_len) {
  if (!valid_tree(tree) || out == NULL || out_len == 0u) {
    return -1;
  }
  return path_from_index(tree, tree->cwd_index, out, out_len);
}
