#include <stdio.h>
#include <string.h>

#include "fs_dir.h"
#include "fs_path.h"

#define TEST_ASSERT(cond, msg)                    \
  do {                                            \
    if (!(cond)) {                                \
      fprintf(stderr, "FAIL: %s\n", (msg));      \
      return 1;                                   \
    }                                             \
  } while (0)

static int test_path_normalization(void) {
  char path[FS_PATH_MAX];

  TEST_ASSERT(fs_path_normalize("//usr///local/../bin/.//", path, sizeof(path)) == 0,
              "normalize absolute path");
  TEST_ASSERT(strcmp(path, "/usr/bin") == 0, "normalized absolute path mismatch");

  TEST_ASSERT(fs_path_resolve("/usr/local", "../../var//log/", path, sizeof(path)) == 0,
              "resolve relative path");
  TEST_ASSERT(strcmp(path, "/var/log") == 0, "resolved path mismatch");

  TEST_ASSERT(fs_path_resolve("/", "../../..", path, sizeof(path)) == 0,
              "resolve above root should clamp to root");
  TEST_ASSERT(strcmp(path, "/") == 0, "root clamping mismatch");

  return 0;
}

static int test_walk_cd_pwd_and_readdir(void) {
  fs_dir_tree_t tree;
  fs_dirent_t entries[8];
  size_t count = 0u;
  char cwd[FS_PATH_MAX];
  int idx = -1;

  fs_dir_init(&tree);

  TEST_ASSERT(fs_dir_mkdir(&tree, "/var") == 0, "mkdir /var");
  TEST_ASSERT(fs_dir_mkdir(&tree, "/usr") == 0, "mkdir /usr");
  TEST_ASSERT(fs_dir_mkdir(&tree, "/tmp") == 0, "mkdir /tmp");
  TEST_ASSERT(fs_dir_mkdir(&tree, "/etc") == 0, "mkdir /etc");
  TEST_ASSERT(fs_dir_mkdir_p(&tree, "/usr/local/bin") == 0, "mkdir -p /usr/local/bin");
  TEST_ASSERT(fs_dir_mkdir_p(&tree, "/var/log/nginx") == 0, "mkdir -p /var/log/nginx");

  TEST_ASSERT(fs_dir_readdir(&tree, "/", entries, 8u, &count) == 0, "readdir /");
  TEST_ASSERT(count == 4u, "root should have four directories");
  TEST_ASSERT(strcmp(entries[0].name, "etc") == 0, "root entry[0] ordering mismatch");
  TEST_ASSERT(strcmp(entries[1].name, "tmp") == 0, "root entry[1] ordering mismatch");
  TEST_ASSERT(strcmp(entries[2].name, "usr") == 0, "root entry[2] ordering mismatch");
  TEST_ASSERT(strcmp(entries[3].name, "var") == 0, "root entry[3] ordering mismatch");

  TEST_ASSERT(fs_dir_readdir(&tree, "/usr", entries, 8u, &count) == 0, "readdir /usr");
  TEST_ASSERT(count == 1u, "/usr should have one child");
  TEST_ASSERT(strcmp(entries[0].name, "local") == 0, "/usr child mismatch");
  TEST_ASSERT(fs_dir_readdir(&tree, "/", NULL, 0u, &count) == 0, "readdir count-only query");
  TEST_ASSERT(count == 4u, "count-only readdir mismatch");
  TEST_ASSERT(fs_dir_readdir(&tree, "/", entries, 2u, &count) != 0,
              "readdir with too-small buffer should fail");
  TEST_ASSERT(count == 4u, "readdir overflow should still report needed count");

  TEST_ASSERT(fs_dir_cd(&tree, "/usr/local/bin") == 0, "cd /usr/local/bin");
  TEST_ASSERT(fs_dir_pwd(&tree, cwd, sizeof(cwd)) == 0, "pwd at /usr/local/bin");
  TEST_ASSERT(strcmp(cwd, "/usr/local/bin") == 0, "pwd mismatch in nested directory");

  TEST_ASSERT(fs_dir_walk(&tree, "../..", &idx) == 0, "walk ../..");
  TEST_ASSERT(fs_dir_cd(&tree, "../..") == 0, "cd ../..");
  TEST_ASSERT(fs_dir_pwd(&tree, cwd, sizeof(cwd)) == 0, "pwd at /usr");
  TEST_ASSERT(strcmp(cwd, "/usr") == 0, "pwd mismatch after relative cd");

  TEST_ASSERT(fs_dir_cd(&tree, "/") == 0, "cd /");
  TEST_ASSERT(fs_dir_cd(&tree, "var/log") == 0, "cd relative into var/log");
  TEST_ASSERT(fs_dir_pwd(&tree, cwd, sizeof(cwd)) == 0, "pwd at /var/log");
  TEST_ASSERT(strcmp(cwd, "/var/log") == 0, "pwd mismatch at /var/log");

  TEST_ASSERT(fs_dir_cd(&tree, "/does-not-exist") != 0, "cd into missing directory should fail");
  TEST_ASSERT(fs_dir_pwd(&tree, cwd, sizeof(cwd)) == 0, "pwd after failed cd");
  TEST_ASSERT(strcmp(cwd, "/var/log") == 0, "failed cd should not change cwd");

  TEST_ASSERT(idx >= 0, "walk should return a valid directory index");
  return 0;
}

int main(void) {
  if (test_path_normalization() != 0) {
    return 1;
  }
  if (test_walk_cd_pwd_and_readdir() != 0) {
    return 1;
  }

  printf("fs dir tests passed\n");
  return 0;
}
