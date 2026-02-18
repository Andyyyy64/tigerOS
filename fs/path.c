#include <stddef.h>
#include <string.h>

#include "fs_path.h"

typedef struct {
  size_t offset;
  size_t len;
} path_segment_t;

static int segment_is_dot(const char *segment, size_t len) {
  return len == 1u && segment[0] == '.';
}

static int segment_is_dotdot(const char *segment, size_t len) {
  return len == 2u && segment[0] == '.' && segment[1] == '.';
}

int fs_path_normalize(const char *path, char *out, size_t out_len) {
  path_segment_t segments[FS_PATH_MAX / 2u];
  char segment_pool[FS_PATH_MAX];
  size_t seg_count = 0u;
  size_t pool_used = 0u;
  size_t i = 0u;
  size_t out_used = 0u;
  int is_absolute = 0;

  if (path == NULL || out == NULL || out_len == 0u || path[0] == '\0') {
    return -1;
  }

  is_absolute = (path[0] == '/');
  while (path[i] != '\0') {
    size_t start = 0u;
    size_t seg_len = 0u;

    while (path[i] == '/') {
      ++i;
    }
    if (path[i] == '\0') {
      break;
    }

    start = i;
    while (path[i] != '\0' && path[i] != '/') {
      ++i;
    }
    seg_len = i - start;

    if (segment_is_dot(&path[start], seg_len)) {
      continue;
    }

    if (segment_is_dotdot(&path[start], seg_len)) {
      if (seg_count > 0u) {
        path_segment_t prev = segments[seg_count - 1u];
        if (!(prev.len == 2u && segment_pool[prev.offset] == '.' &&
              segment_pool[prev.offset + 1u] == '.')) {
          --seg_count;
          continue;
        }
      }

      if (is_absolute) {
        continue;
      }
    }

    if (seg_count >= (sizeof(segments) / sizeof(segments[0])) ||
        seg_len >= FS_PATH_MAX || pool_used + seg_len + 1u > sizeof(segment_pool)) {
      return -1;
    }

    memcpy(&segment_pool[pool_used], &path[start], seg_len);
    segment_pool[pool_used + seg_len] = '\0';
    segments[seg_count].offset = pool_used;
    segments[seg_count].len = seg_len;
    ++seg_count;
    pool_used += seg_len + 1u;
  }

  if (is_absolute) {
    if (out_used + 2u > out_len) {
      return -1;
    }
    out[out_used++] = '/';
  }

  if (seg_count == 0u) {
    if (!is_absolute) {
      if (out_len < 2u) {
        return -1;
      }
      out[0] = '.';
      out[1] = '\0';
      return 0;
    }
    out[out_used] = '\0';
    return 0;
  }

  for (i = 0u; i < seg_count; ++i) {
    const path_segment_t seg = segments[i];

    if (i > 0u) {
      if (out_used + 2u > out_len) {
        return -1;
      }
      out[out_used++] = '/';
    }

    if (out_used + seg.len + 1u > out_len) {
      return -1;
    }
    memcpy(&out[out_used], &segment_pool[seg.offset], seg.len);
    out_used += seg.len;
  }

  out[out_used] = '\0';
  return 0;
}

int fs_path_resolve(const char *cwd, const char *path, char *out, size_t out_len) {
  char cwd_normalized[FS_PATH_MAX];
  char joined[FS_PATH_MAX];
  size_t cwd_len = 0u;
  size_t path_len = 0u;

  if (cwd == NULL || path == NULL || out == NULL || out_len == 0u) {
    return -1;
  }

  if (fs_path_normalize(cwd, cwd_normalized, sizeof(cwd_normalized)) != 0 ||
      cwd_normalized[0] != '/') {
    return -1;
  }

  if (path[0] == '/') {
    return fs_path_normalize(path, out, out_len);
  }

  if (path[0] == '\0') {
    return fs_path_normalize(cwd_normalized, out, out_len);
  }

  cwd_len = strlen(cwd_normalized);
  path_len = strlen(path);
  if (cwd_len + path_len + 2u > sizeof(joined)) {
    return -1;
  }

  if (strcmp(cwd_normalized, "/") == 0) {
    memcpy(joined, "/", 2u);
    memcpy(&joined[1], path, path_len + 1u);
  } else {
    memcpy(joined, cwd_normalized, cwd_len);
    joined[cwd_len] = '/';
    memcpy(&joined[cwd_len + 1u], path, path_len + 1u);
  }

  return fs_path_normalize(joined, out, out_len);
}
