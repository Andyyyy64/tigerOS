#include <stddef.h>

#include "fs_dir.h"
#include "fs_path.h"
#include "path_state.h"

typedef struct {
  const char *path;
  const char *content;
} path_state_file_t;

static path_state_context_t g_default_context;

static const path_state_file_t g_seed_files[] = {
    {"/hello.txt", "hello from shell fs\n"},
    {"/etc/motd", "openTiger shell filesystem\n"},
    {"/home/readme.txt", "use ls, cat, pwd, cd, mkdir\n"},
};

static size_t ps_strlen(const char *s) {
  size_t len = 0u;
  if (s == NULL) {
    return 0u;
  }
  while (s[len] != '\0') {
    ++len;
  }
  return len;
}

static int ps_strcmp(const char *a, const char *b) {
  size_t i = 0u;

  if (a == NULL || b == NULL) {
    if (a == b) {
      return 0;
    }
    return (a == NULL) ? -1 : 1;
  }

  while (a[i] != '\0' && b[i] != '\0') {
    if (a[i] != b[i]) {
      return (int)((unsigned char)a[i] - (unsigned char)b[i]);
    }
    ++i;
  }

  return (int)((unsigned char)a[i] - (unsigned char)b[i]);
}

static int ps_strcpy(char *dst, size_t dst_cap, const char *src) {
  size_t i = 0u;

  if (dst == NULL || src == NULL || dst_cap == 0u) {
    return -1;
  }

  while (src[i] != '\0') {
    if (i + 1u >= dst_cap) {
      return -1;
    }
    dst[i] = src[i];
    ++i;
  }

  dst[i] = '\0';
  return 0;
}

static int path_basename(const char *path, char *out, size_t out_len) {
  size_t i = 0u;
  size_t last_slash = 0u;

  if (path == NULL || out == NULL || out_len == 0u || path[0] != '/') {
    return -1;
  }

  while (path[i] != '\0') {
    if (path[i] == '/') {
      last_slash = i;
    }
    ++i;
  }

  if (last_slash + 1u >= i) {
    return -1;
  }

  return ps_strcpy(out, out_len, &path[last_slash + 1u]);
}

static int path_parent(const char *path, char *out, size_t out_len) {
  size_t i = 0u;
  size_t last_slash = 0u;
  size_t j = 0u;

  if (path == NULL || out == NULL || out_len == 0u || path[0] != '/') {
    return -1;
  }

  while (path[i] != '\0') {
    if (path[i] == '/') {
      last_slash = i;
    }
    ++i;
  }

  if (last_slash == 0u) {
    if (out_len < 2u) {
      return -1;
    }
    out[0] = '/';
    out[1] = '\0';
    return 0;
  }

  if (last_slash + 1u > out_len) {
    return -1;
  }

  while (j < last_slash) {
    out[j] = path[j];
    ++j;
  }
  out[j] = '\0';
  return 0;
}

static int resolve_to_absolute(path_state_context_t *context,
                               const char *path,
                               char *out,
                               size_t out_len) {
  char cwd[FS_PATH_MAX];
  const char *input = path;

  if (context == NULL || out == NULL || out_len == 0u) {
    return -1;
  }
  if (input == NULL || input[0] == '\0') {
    input = ".";
  }

  if (fs_dir_pwd(&context->dir_tree, cwd, sizeof(cwd)) != 0) {
    return -1;
  }

  return fs_path_resolve(cwd, input, out, out_len);
}

static int file_index_by_absolute(const char *absolute_path) {
  size_t i = 0u;
  for (i = 0u; i < (sizeof(g_seed_files) / sizeof(g_seed_files[0])); ++i) {
    if (ps_strcmp(g_seed_files[i].path, absolute_path) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int file_is_child_of_dir(const path_state_file_t *file, const char *dir_absolute) {
  char parent[FS_PATH_MAX];

  if (file == NULL || dir_absolute == NULL) {
    return 0;
  }

  if (path_parent(file->path, parent, sizeof(parent)) != 0) {
    return 0;
  }

  return ps_strcmp(parent, dir_absolute) == 0;
}

static int entry_insert_sorted(path_state_entry_t *entries,
                               size_t *io_count,
                               size_t max_entries,
                               const char *name,
                               path_state_entry_kind_t kind) {
  size_t insert_at = 0u;
  size_t count = 0u;

  if (entries == NULL || io_count == NULL || name == NULL) {
    return -1;
  }
  count = *io_count;
  if (count >= max_entries) {
    return -1;
  }
  if (ps_strlen(name) > FS_DIR_NAME_MAX) {
    return -1;
  }

  while (insert_at < count && ps_strcmp(entries[insert_at].name, name) < 0) {
    ++insert_at;
  }

  while (count > insert_at) {
    entries[count] = entries[count - 1u];
    --count;
  }

  if (ps_strcpy(entries[insert_at].name, sizeof(entries[insert_at].name), name) != 0) {
    return -1;
  }
  entries[insert_at].kind = kind;
  *io_count += 1u;
  return 0;
}

static int ensure_initialized(path_state_context_t *context) {
  size_t i = 0u;

  if (context == NULL) {
    return -1;
  }

  if (context->ready != 0) {
    return 0;
  }

  fs_dir_init(&context->dir_tree);
  if (fs_dir_mkdir(&context->dir_tree, "/etc") != 0 || fs_dir_mkdir(&context->dir_tree, "/home") != 0 ||
      fs_dir_mkdir(&context->dir_tree, "/tmp") != 0) {
    return -1;
  }

  for (i = 0u; i < (sizeof(g_seed_files) / sizeof(g_seed_files[0])); ++i) {
    char parent[FS_PATH_MAX];
    if (path_parent(g_seed_files[i].path, parent, sizeof(parent)) != 0) {
      return -1;
    }
    if (ps_strcmp(parent, "/") != 0 && fs_dir_mkdir_p(&context->dir_tree, parent) != 0) {
      return -1;
    }
  }

  context->ready = 1;
  return 0;
}

void path_state_context_init(path_state_context_t *context) {
  if (context == NULL) {
    return;
  }

  context->ready = 0;
  (void)ensure_initialized(context);
}

int path_state_pwd_ctx(path_state_context_t *context, char *out, size_t out_len) {
  if (ensure_initialized(context) != 0) {
    return -1;
  }
  return fs_dir_pwd(&context->dir_tree, out, out_len);
}

int path_state_cd_ctx(path_state_context_t *context, const char *path) {
  if (ensure_initialized(context) != 0) {
    return -1;
  }
  if (path == NULL) {
    return -1;
  }
  return fs_dir_cd(&context->dir_tree, path);
}

int path_state_mkdir_ctx(path_state_context_t *context, const char *path) {
  char absolute[FS_PATH_MAX];

  if (ensure_initialized(context) != 0) {
    return -1;
  }
  if (path == NULL || resolve_to_absolute(context, path, absolute, sizeof(absolute)) != 0) {
    return -1;
  }
  if (absolute[0] == '/' && absolute[1] == '\0') {
    return -1;
  }
  if (file_index_by_absolute(absolute) >= 0) {
    return -1;
  }

  return fs_dir_mkdir(&context->dir_tree, absolute);
}

int path_state_ls_ctx(path_state_context_t *context,
                      const char *path,
                      path_state_entry_t *entries,
                      size_t max_entries,
                      size_t *out_count) {
  char absolute[FS_PATH_MAX];
  size_t count = 0u;
  int is_file_path = 0;

  if (out_count == NULL) {
    return -1;
  }
  *out_count = 0u;

  if (ensure_initialized(context) != 0) {
    return -1;
  }
  if (resolve_to_absolute(context, path, absolute, sizeof(absolute)) != 0) {
    return -1;
  }

  if (file_index_by_absolute(absolute) >= 0) {
    is_file_path = 1;
  }

  if (is_file_path != 0) {
    char basename[FS_DIR_NAME_MAX + 1u];

    if (path_basename(absolute, basename, sizeof(basename)) != 0) {
      return -1;
    }
    if (entries != NULL &&
        entry_insert_sorted(entries, &count, max_entries, basename, PATH_STATE_ENTRY_FILE) != 0) {
      return -1;
    }
    if (entries == NULL) {
      count = 1u;
    }
    *out_count = count;
    return 0;
  }

  {
    fs_dirent_t dir_entries[FS_DIR_MAX_NODES];
    size_t dir_count = 0u;
    size_t i = 0u;

    if (fs_dir_readdir(&context->dir_tree, absolute, dir_entries, FS_DIR_MAX_NODES, &dir_count) != 0) {
      return -1;
    }

    for (i = 0u; i < dir_count; ++i) {
      if (entries != NULL &&
          entry_insert_sorted(entries, &count, max_entries, dir_entries[i].name,
                              PATH_STATE_ENTRY_DIR) != 0) {
        return -1;
      }
      if (entries == NULL) {
        count += 1u;
      }
    }
  }

  {
    size_t i = 0u;
    for (i = 0u; i < (sizeof(g_seed_files) / sizeof(g_seed_files[0])); ++i) {
      if (file_is_child_of_dir(&g_seed_files[i], absolute) != 0) {
        char basename[FS_DIR_NAME_MAX + 1u];
        if (path_basename(g_seed_files[i].path, basename, sizeof(basename)) != 0) {
          return -1;
        }
        if (entries != NULL &&
            entry_insert_sorted(entries, &count, max_entries, basename, PATH_STATE_ENTRY_FILE) !=
                0) {
          return -1;
        }
        if (entries == NULL) {
          count += 1u;
        }
      }
    }
  }

  *out_count = count;
  return 0;
}

int path_state_cat_ctx(path_state_context_t *context, const char *path, const char **out_contents) {
  char absolute[FS_PATH_MAX];
  int file_index;

  if (out_contents == NULL) {
    return -1;
  }
  *out_contents = NULL;

  if (ensure_initialized(context) != 0) {
    return -1;
  }
  if (path == NULL || resolve_to_absolute(context, path, absolute, sizeof(absolute)) != 0) {
    return -1;
  }

  file_index = file_index_by_absolute(absolute);
  if (file_index < 0) {
    return -1;
  }

  *out_contents = g_seed_files[(size_t)file_index].content;
  return 0;
}

void path_state_init(void) { path_state_context_init(&g_default_context); }

int path_state_pwd(char *out, size_t out_len) {
  return path_state_pwd_ctx(&g_default_context, out, out_len);
}

int path_state_cd(const char *path) { return path_state_cd_ctx(&g_default_context, path); }

int path_state_mkdir(const char *path) {
  return path_state_mkdir_ctx(&g_default_context, path);
}

int path_state_ls(const char *path,
                  path_state_entry_t *entries,
                  size_t max_entries,
                  size_t *out_count) {
  return path_state_ls_ctx(&g_default_context, path, entries, max_entries, out_count);
}

int path_state_cat(const char *path, const char **out_contents) {
  return path_state_cat_ctx(&g_default_context, path, out_contents);
}
