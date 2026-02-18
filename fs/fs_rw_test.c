#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"

static int expect_ok(int rc, const char *step) {
  if (rc != 0) {
    fprintf(stderr, "FAIL: %s (rc=%d)\n", step, rc);
    return 1;
  }
  return 0;
}

static int write_file(fs_handle_t *fs, const char *name, const void *data, size_t len) {
  int fd;
  size_t written = 0;

  fd = fs_open(fs, name, FS_O_WRITE | FS_O_CREATE | FS_O_TRUNC);
  if (fd < 0) {
    fprintf(stderr, "FAIL: open-for-write %s (fd=%d)\n", name, fd);
    return 1;
  }

  if (expect_ok(fs_write(fs, fd, data, len, &written), "fs_write") != 0) {
    (void)fs_close(fs, fd);
    return 1;
  }
  if (written != len) {
    fprintf(stderr, "FAIL: short write for %s (%zu/%zu)\n", name, written, len);
    (void)fs_close(fs, fd);
    return 1;
  }
  if (expect_ok(fs_close(fs, fd), "fs_close(write)") != 0) {
    return 1;
  }

  return 0;
}

static int read_file_exact(fs_handle_t *fs, const char *name, void *buf, size_t len) {
  int fd;
  size_t got = 0;

  fd = fs_open(fs, name, FS_O_READ);
  if (fd < 0) {
    fprintf(stderr, "FAIL: open-for-read %s (fd=%d)\n", name, fd);
    return 1;
  }

  if (expect_ok(fs_read(fs, fd, buf, len, &got), "fs_read") != 0) {
    (void)fs_close(fs, fd);
    return 1;
  }
  if (got != len) {
    fprintf(stderr, "FAIL: short read for %s (%zu/%zu)\n", name, got, len);
    (void)fs_close(fs, fd);
    return 1;
  }
  if (expect_ok(fs_close(fs, fd), "fs_close(read)") != 0) {
    return 1;
  }

  return 0;
}

int main(int argc, char **argv) {
  fs_handle_t fs;
  const char *image_path;
  const char *text_v1 = "bootstrapped file content";
  const char *text_v2 = "updated file content after truncation";
  const char *text_v3 = "updated DATA content after truncation";
  char text_buf[128];
  uint8_t pattern[1300];
  uint8_t pattern_read[1300];
  uint8_t sparse_read[603];
  const char sparse_tail[] = "XYZ";
  int fd;
  size_t written = 0;
  size_t i = 0;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <image-path>\n", argv[0]);
    return 2;
  }
  image_path = argv[1];

  fs_init(&fs);
  if (expect_ok(fs_mount(&fs, image_path), "fs_mount initial") != 0) {
    return 1;
  }

  if (write_file(&fs, "note.txt", text_v1, strlen(text_v1)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  memset(text_buf, 0, sizeof(text_buf));
  if (read_file_exact(&fs, "note.txt", text_buf, strlen(text_v1)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  if (memcmp(text_buf, text_v1, strlen(text_v1)) != 0) {
    fprintf(stderr, "FAIL: note.txt content mismatch after first write\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  if (write_file(&fs, "note.txt", text_v2, strlen(text_v2)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  memset(text_buf, 0, sizeof(text_buf));
  if (read_file_exact(&fs, "note.txt", text_buf, strlen(text_v2)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  if (memcmp(text_buf, text_v2, strlen(text_v2)) != 0) {
    fprintf(stderr, "FAIL: note.txt content mismatch after overwrite\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  fd = fs_open(&fs, "note.txt", FS_O_WRITE);
  if (fd < 0) {
    fprintf(stderr, "FAIL: open-for-seek-write note.txt (fd=%d)\n", fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (expect_ok(fs_seek(&fs, fd, 8u), "fs_seek(note.txt)") != 0) {
    (void)fs_close(&fs, fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (expect_ok(fs_write(&fs, fd, "DATA", 4u, &written), "fs_write(note patch)") != 0) {
    (void)fs_close(&fs, fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (written != 4u) {
    fprintf(stderr, "FAIL: short patch write for note.txt (%zu/4)\n", written);
    (void)fs_close(&fs, fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (expect_ok(fs_close(&fs, fd), "fs_close(seek-write)") != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }

  memset(text_buf, 0, sizeof(text_buf));
  if (read_file_exact(&fs, "note.txt", text_buf, strlen(text_v3)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  if (memcmp(text_buf, text_v3, strlen(text_v3)) != 0) {
    fprintf(stderr, "FAIL: note.txt content mismatch after seek overwrite\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  for (i = 0; i < sizeof(pattern); ++i) {
    pattern[i] = (uint8_t)('A' + (i % 26u));
  }
  if (write_file(&fs, "blob.bin", pattern, sizeof(pattern)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  memset(pattern_read, 0, sizeof(pattern_read));
  if (read_file_exact(&fs, "blob.bin", pattern_read, sizeof(pattern_read)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  if (memcmp(pattern, pattern_read, sizeof(pattern)) != 0) {
    fprintf(stderr, "FAIL: blob.bin content mismatch\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  fd = fs_open(&fs, "sparse.bin", FS_O_WRITE | FS_O_CREATE | FS_O_TRUNC);
  if (fd < 0) {
    fprintf(stderr, "FAIL: open-for-write sparse.bin (fd=%d)\n", fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (expect_ok(fs_seek(&fs, fd, 600u), "fs_seek(sparse.bin)") != 0) {
    (void)fs_close(&fs, fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (expect_ok(fs_write(&fs, fd, sparse_tail, sizeof(sparse_tail) - 1u, &written),
                "fs_write(sparse tail)") != 0) {
    (void)fs_close(&fs, fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (written != (sizeof(sparse_tail) - 1u)) {
    fprintf(stderr, "FAIL: short sparse write (%zu/%zu)\n", written, sizeof(sparse_tail) - 1u);
    (void)fs_close(&fs, fd);
    (void)fs_unmount(&fs);
    return 1;
  }
  if (expect_ok(fs_close(&fs, fd), "fs_close(sparse-write)") != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }

  memset(sparse_read, 0xaa, sizeof(sparse_read));
  if (read_file_exact(&fs, "sparse.bin", sparse_read, sizeof(sparse_read)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  for (i = 0; i < 600u; ++i) {
    if (sparse_read[i] != 0u) {
      fprintf(stderr, "FAIL: sparse.bin gap byte %zu expected zero got 0x%02x\n", i,
              sparse_read[i]);
      (void)fs_unmount(&fs);
      return 1;
    }
  }
  if (memcmp(sparse_read + 600u, sparse_tail, sizeof(sparse_tail) - 1u) != 0) {
    fprintf(stderr, "FAIL: sparse.bin tail mismatch\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  if (expect_ok(fs_unmount(&fs), "fs_unmount first") != 0) {
    return 1;
  }
  if (expect_ok(fs_mount(&fs, image_path), "fs_mount remount") != 0) {
    return 1;
  }

  memset(text_buf, 0, sizeof(text_buf));
  if (read_file_exact(&fs, "note.txt", text_buf, strlen(text_v3)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  if (memcmp(text_buf, text_v3, strlen(text_v3)) != 0) {
    fprintf(stderr, "FAIL: note.txt content mismatch after remount\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  memset(pattern_read, 0, sizeof(pattern_read));
  if (read_file_exact(&fs, "blob.bin", pattern_read, sizeof(pattern_read)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  if (memcmp(pattern, pattern_read, sizeof(pattern)) != 0) {
    fprintf(stderr, "FAIL: blob.bin content mismatch after remount\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  memset(sparse_read, 0xaa, sizeof(sparse_read));
  if (read_file_exact(&fs, "sparse.bin", sparse_read, sizeof(sparse_read)) != 0) {
    (void)fs_unmount(&fs);
    return 1;
  }
  for (i = 0; i < 600u; ++i) {
    if (sparse_read[i] != 0u) {
      fprintf(stderr, "FAIL: sparse.bin gap byte %zu changed after remount\n", i);
      (void)fs_unmount(&fs);
      return 1;
    }
  }
  if (memcmp(sparse_read + 600u, sparse_tail, sizeof(sparse_tail) - 1u) != 0) {
    fprintf(stderr, "FAIL: sparse.bin tail mismatch after remount\n");
    (void)fs_unmount(&fs);
    return 1;
  }

  if (expect_ok(fs_unmount(&fs), "fs_unmount final") != 0) {
    return 1;
  }

  printf("PASS: mount/open/read/write/close checks completed\n");
  return 0;
}
