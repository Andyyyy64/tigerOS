#ifndef FS_H
#define FS_H

#include <stddef.h>
#include <stdint.h>

#define FS_BLOCK_SIZE 512u
#define FS_TOTAL_BLOCKS 256u
#define FS_MAX_FILES 32u
#define FS_MAX_OPEN_FILES 16u
#define FS_MAX_NAME_LEN 31u

#define FS_O_READ (1u << 0)
#define FS_O_WRITE (1u << 1)
#define FS_O_CREATE (1u << 2)
#define FS_O_TRUNC (1u << 3)

typedef struct {
  uint8_t in_use;
  uint8_t dir_index;
  uint16_t reserved;
  uint32_t offset;
  uint32_t flags;
} fs_open_file_t;

typedef struct {
  void *image_file;
  uint32_t mounted;
  uint32_t data_blocks;
  uint32_t block_size;
  uint32_t data_start_block;
  uint8_t dir_region[FS_MAX_FILES * 64u];
  uint32_t fat[FS_TOTAL_BLOCKS];
  fs_open_file_t open_files[FS_MAX_OPEN_FILES];
} fs_handle_t;

void fs_init(fs_handle_t *fs);
int fs_format_image(const char *image_path);
int fs_mount(fs_handle_t *fs, const char *image_path);
int fs_unmount(fs_handle_t *fs);
int fs_open(fs_handle_t *fs, const char *name, uint32_t flags);
int fs_close(fs_handle_t *fs, int fd);
int fs_read(fs_handle_t *fs, int fd, void *buf, size_t len, size_t *bytes_read);
int fs_write(fs_handle_t *fs, int fd, const void *buf, size_t len, size_t *bytes_written);
int fs_seek(fs_handle_t *fs, int fd, uint32_t offset);

#endif
