#ifndef FS_PATH_H
#define FS_PATH_H

#include <stddef.h>

#define FS_PATH_MAX 256u

int fs_path_normalize(const char *path, char *out, size_t out_len);
int fs_path_resolve(const char *cwd, const char *path, char *out, size_t out_len);

#endif
