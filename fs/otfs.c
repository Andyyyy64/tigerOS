#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "fs.h"

#define FS_DIR_ENTRY_SIZE 64u
#define FS_SUPERBLOCK_SIZE 64u
#define FS_VERSION 1u
#define FS_DIR_START_BLOCK 1u
#define FS_DIR_BLOCK_COUNT 4u
#define FS_FAT_START_BLOCK (FS_DIR_START_BLOCK + FS_DIR_BLOCK_COUNT)
#define FS_FAT_BLOCK_COUNT 2u
#define FS_DATA_START_BLOCK (FS_FAT_START_BLOCK + FS_FAT_BLOCK_COUNT)
#define FS_DATA_BLOCK_COUNT (FS_TOTAL_BLOCKS - FS_DATA_START_BLOCK)

#define FAT_FREE 0xffffffffu
#define FAT_END 0xfffffffeu

#define FS_OK 0
#define FS_ERR_ARG -1
#define FS_ERR_IO -2
#define FS_ERR_STATE -3
#define FS_ERR_NOT_FOUND -4
#define FS_ERR_NO_SPACE -5

typedef struct __attribute__((packed)) {
  uint8_t magic[8];
  uint32_t version;
  uint32_t block_size;
  uint32_t total_blocks;
  uint32_t dir_start_block;
  uint32_t dir_block_count;
  uint32_t fat_start_block;
  uint32_t fat_block_count;
  uint32_t data_start_block;
  uint32_t data_block_count;
  uint32_t max_files;
  uint32_t reserved[4];
} fs_superblock_disk_t;

typedef struct __attribute__((packed)) {
  uint8_t used;
  uint8_t reserved0[3];
  char name[32];
  uint32_t first_block;
  uint32_t size_bytes;
  uint8_t reserved1[20];
} fs_dir_entry_disk_t;

_Static_assert(sizeof(fs_superblock_disk_t) == FS_SUPERBLOCK_SIZE,
               "superblock size must be 64 bytes");
_Static_assert(sizeof(fs_dir_entry_disk_t) == FS_DIR_ENTRY_SIZE,
               "directory entry size must be 64 bytes");

static const uint8_t k_magic[8] = {'O', 'T', 'F', 'S', 'v', '1', 0, 0};

static uint64_t block_offset(uint32_t block_index) {
  return (uint64_t)block_index * (uint64_t)FS_BLOCK_SIZE;
}

static FILE *get_file(fs_handle_t *fs) { return (FILE *)fs->image_file; }

static fs_dir_entry_disk_t *dir_entries(fs_handle_t *fs) {
  return (fs_dir_entry_disk_t *)fs->dir_region;
}

static int seek_and_read(FILE *file, uint64_t offset, void *buf, size_t len) {
  if (fseek(file, (long)offset, SEEK_SET) != 0) {
    return FS_ERR_IO;
  }
  if (fread(buf, 1, len, file) != len) {
    return FS_ERR_IO;
  }
  return FS_OK;
}

static int seek_and_write(FILE *file, uint64_t offset, const void *buf, size_t len) {
  if (fseek(file, (long)offset, SEEK_SET) != 0) {
    return FS_ERR_IO;
  }
  if (fwrite(buf, 1, len, file) != len) {
    return FS_ERR_IO;
  }
  if (fflush(file) != 0) {
    return FS_ERR_IO;
  }
  return FS_OK;
}

static int sync_metadata(fs_handle_t *fs) {
  uint8_t fat_bytes[FS_FAT_BLOCK_COUNT * FS_BLOCK_SIZE];
  FILE *file = get_file(fs);

  memset(fat_bytes, 0xff, sizeof(fat_bytes));
  memcpy(fat_bytes, fs->fat, FS_DATA_BLOCK_COUNT * sizeof(uint32_t));

  if (seek_and_write(file, block_offset(FS_DIR_START_BLOCK), fs->dir_region,
                     sizeof(fs->dir_region)) != FS_OK) {
    return FS_ERR_IO;
  }

  if (seek_and_write(file, block_offset(FS_FAT_START_BLOCK), fat_bytes,
                     sizeof(fat_bytes)) != FS_OK) {
    return FS_ERR_IO;
  }

  return FS_OK;
}

static bool valid_block_index(uint32_t index) { return index < FS_DATA_BLOCK_COUNT; }

static int allocate_data_block(fs_handle_t *fs, uint32_t *out_block_index) {
  uint8_t zeros[FS_BLOCK_SIZE];
  uint32_t index = 0;

  memset(zeros, 0, sizeof(zeros));
  for (index = 0; index < FS_DATA_BLOCK_COUNT; ++index) {
    if (fs->fat[index] == FAT_FREE) {
      fs->fat[index] = FAT_END;
      if (seek_and_write(get_file(fs), block_offset(FS_DATA_START_BLOCK + index), zeros,
                         sizeof(zeros)) != FS_OK) {
        return FS_ERR_IO;
      }
      *out_block_index = index;
      return FS_OK;
    }
  }
  return FS_ERR_NO_SPACE;
}

static int release_chain(fs_handle_t *fs, uint32_t first_block) {
  uint32_t cur = first_block;
  uint32_t seen = 0;

  while (cur != FAT_END) {
    uint32_t next;
    if (!valid_block_index(cur)) {
      return FS_ERR_STATE;
    }
    if (seen++ > FS_DATA_BLOCK_COUNT) {
      return FS_ERR_STATE;
    }
    next = fs->fat[cur];
    fs->fat[cur] = FAT_FREE;
    cur = next;
  }
  return FS_OK;
}

static int resolve_data_block(fs_handle_t *fs,
                              fs_dir_entry_disk_t *entry,
                              uint32_t logical_block_index,
                              bool allocate,
                              uint32_t *out_block_index) {
  uint32_t cur;
  uint32_t step;

  if (entry->first_block == FAT_END) {
    uint32_t first = FAT_END;
    if (!allocate) {
      return FS_ERR_NOT_FOUND;
    }
    if (allocate_data_block(fs, &first) != FS_OK) {
      return FS_ERR_NO_SPACE;
    }
    entry->first_block = first;
  }

  cur = entry->first_block;
  if (!valid_block_index(cur)) {
    return FS_ERR_STATE;
  }

  for (step = 0; step < logical_block_index; ++step) {
    uint32_t next = fs->fat[cur];
    if (next == FAT_END) {
      if (!allocate) {
        return FS_ERR_NOT_FOUND;
      }
      if (allocate_data_block(fs, &next) != FS_OK) {
        return FS_ERR_NO_SPACE;
      }
      fs->fat[cur] = next;
    }

    if (!valid_block_index(next)) {
      return FS_ERR_STATE;
    }
    cur = next;
  }

  *out_block_index = cur;
  return FS_OK;
}

static int read_data_block(fs_handle_t *fs, uint32_t data_block_index, uint8_t *out) {
  if (!valid_block_index(data_block_index)) {
    return FS_ERR_ARG;
  }
  return seek_and_read(get_file(fs), block_offset(FS_DATA_START_BLOCK + data_block_index), out,
                       FS_BLOCK_SIZE);
}

static int write_data_block(fs_handle_t *fs, uint32_t data_block_index, const uint8_t *data) {
  if (!valid_block_index(data_block_index)) {
    return FS_ERR_ARG;
  }
  return seek_and_write(get_file(fs), block_offset(FS_DATA_START_BLOCK + data_block_index), data,
                        FS_BLOCK_SIZE);
}

static int find_dir_entry(fs_handle_t *fs, const char *name) {
  fs_dir_entry_disk_t *entries = dir_entries(fs);
  uint32_t i = 0;

  for (i = 0; i < FS_MAX_FILES; ++i) {
    if (entries[i].used != 0 && strncmp(entries[i].name, name, FS_MAX_NAME_LEN + 1u) == 0) {
      return (int)i;
    }
  }
  return -1;
}

static int alloc_dir_entry(fs_handle_t *fs, const char *name) {
  fs_dir_entry_disk_t *entries = dir_entries(fs);
  uint32_t i = 0;

  for (i = 0; i < FS_MAX_FILES; ++i) {
    if (entries[i].used == 0) {
      memset(&entries[i], 0, sizeof(entries[i]));
      entries[i].used = 1;
      entries[i].first_block = FAT_END;
      strncpy(entries[i].name, name, FS_MAX_NAME_LEN);
      entries[i].name[FS_MAX_NAME_LEN] = '\0';
      return (int)i;
    }
  }
  return -1;
}

static bool valid_fd(int fd) { return fd >= 0 && fd < (int)FS_MAX_OPEN_FILES; }

static int alloc_fd(fs_handle_t *fs) {
  int fd = 0;
  for (fd = 0; fd < (int)FS_MAX_OPEN_FILES; ++fd) {
    if (fs->open_files[fd].in_use == 0) {
      return fd;
    }
  }
  return -1;
}

static int validate_common(fs_handle_t *fs) {
  if (fs == NULL) {
    return FS_ERR_ARG;
  }
  if (fs->mounted == 0u) {
    return FS_ERR_STATE;
  }
  return FS_OK;
}

void fs_init(fs_handle_t *fs) {
  if (fs != NULL) {
    memset(fs, 0, sizeof(*fs));
  }
}

int fs_format_image(const char *image_path) {
  fs_superblock_disk_t sb;
  fs_dir_entry_disk_t entries[FS_MAX_FILES];
  uint8_t fat_bytes[FS_FAT_BLOCK_COUNT * FS_BLOCK_SIZE];
  uint8_t zeros[FS_BLOCK_SIZE];
  uint32_t i = 0;
  FILE *file;

  if (image_path == NULL) {
    return FS_ERR_ARG;
  }

  file = fopen(image_path, "wb+");
  if (file == NULL) {
    return FS_ERR_IO;
  }

  memset(zeros, 0, sizeof(zeros));
  for (i = 0; i < FS_TOTAL_BLOCKS; ++i) {
    if (fwrite(zeros, 1, sizeof(zeros), file) != sizeof(zeros)) {
      fclose(file);
      return FS_ERR_IO;
    }
  }

  memset(&sb, 0, sizeof(sb));
  memcpy(sb.magic, k_magic, sizeof(k_magic));
  sb.version = FS_VERSION;
  sb.block_size = FS_BLOCK_SIZE;
  sb.total_blocks = FS_TOTAL_BLOCKS;
  sb.dir_start_block = FS_DIR_START_BLOCK;
  sb.dir_block_count = FS_DIR_BLOCK_COUNT;
  sb.fat_start_block = FS_FAT_START_BLOCK;
  sb.fat_block_count = FS_FAT_BLOCK_COUNT;
  sb.data_start_block = FS_DATA_START_BLOCK;
  sb.data_block_count = FS_DATA_BLOCK_COUNT;
  sb.max_files = FS_MAX_FILES;

  if (seek_and_write(file, 0, &sb, sizeof(sb)) != FS_OK) {
    fclose(file);
    return FS_ERR_IO;
  }

  memset(entries, 0, sizeof(entries));
  for (i = 0; i < FS_MAX_FILES; ++i) {
    entries[i].first_block = FAT_END;
  }
  if (seek_and_write(file, block_offset(FS_DIR_START_BLOCK), entries, sizeof(entries)) != FS_OK) {
    fclose(file);
    return FS_ERR_IO;
  }

  memset(fat_bytes, 0xff, sizeof(fat_bytes));
  if (seek_and_write(file, block_offset(FS_FAT_START_BLOCK), fat_bytes, sizeof(fat_bytes)) !=
      FS_OK) {
    fclose(file);
    return FS_ERR_IO;
  }

  if (fclose(file) != 0) {
    return FS_ERR_IO;
  }
  return FS_OK;
}

int fs_mount(fs_handle_t *fs, const char *image_path) {
  fs_superblock_disk_t sb;
  uint8_t fat_bytes[FS_FAT_BLOCK_COUNT * FS_BLOCK_SIZE];
  FILE *file;

  if (fs == NULL || image_path == NULL) {
    return FS_ERR_ARG;
  }

  fs_init(fs);
  file = fopen(image_path, "rb+");
  if (file == NULL) {
    return FS_ERR_IO;
  }

  if (seek_and_read(file, 0, &sb, sizeof(sb)) != FS_OK) {
    fclose(file);
    return FS_ERR_IO;
  }

  if (memcmp(sb.magic, k_magic, sizeof(k_magic)) != 0 || sb.version != FS_VERSION ||
      sb.block_size != FS_BLOCK_SIZE || sb.total_blocks != FS_TOTAL_BLOCKS ||
      sb.dir_start_block != FS_DIR_START_BLOCK || sb.dir_block_count != FS_DIR_BLOCK_COUNT ||
      sb.fat_start_block != FS_FAT_START_BLOCK || sb.fat_block_count != FS_FAT_BLOCK_COUNT ||
      sb.data_start_block != FS_DATA_START_BLOCK || sb.data_block_count != FS_DATA_BLOCK_COUNT ||
      sb.max_files != FS_MAX_FILES) {
    fclose(file);
    return FS_ERR_STATE;
  }

  if (seek_and_read(file, block_offset(FS_DIR_START_BLOCK), fs->dir_region,
                    sizeof(fs->dir_region)) != FS_OK) {
    fclose(file);
    return FS_ERR_IO;
  }

  if (seek_and_read(file, block_offset(FS_FAT_START_BLOCK), fat_bytes, sizeof(fat_bytes)) !=
      FS_OK) {
    fclose(file);
    return FS_ERR_IO;
  }
  memcpy(fs->fat, fat_bytes, FS_DATA_BLOCK_COUNT * sizeof(uint32_t));

  fs->image_file = file;
  fs->mounted = 1u;
  fs->block_size = FS_BLOCK_SIZE;
  fs->data_start_block = FS_DATA_START_BLOCK;
  fs->data_blocks = FS_DATA_BLOCK_COUNT;

  return FS_OK;
}

int fs_unmount(fs_handle_t *fs) {
  FILE *file;
  if (validate_common(fs) != FS_OK) {
    return FS_ERR_STATE;
  }

  if (sync_metadata(fs) != FS_OK) {
    return FS_ERR_IO;
  }

  file = get_file(fs);
  if (fclose(file) != 0) {
    return FS_ERR_IO;
  }

  fs_init(fs);
  return FS_OK;
}

int fs_open(fs_handle_t *fs, const char *name, uint32_t flags) {
  fs_dir_entry_disk_t *entries;
  int dir_index;
  int fd;

  if (validate_common(fs) != FS_OK || name == NULL) {
    return FS_ERR_ARG;
  }
  if ((flags & (FS_O_READ | FS_O_WRITE)) == 0u) {
    return FS_ERR_ARG;
  }
  if (name[0] == '\0' || strlen(name) > FS_MAX_NAME_LEN) {
    return FS_ERR_ARG;
  }
  if ((flags & FS_O_TRUNC) != 0u && (flags & FS_O_WRITE) == 0u) {
    return FS_ERR_ARG;
  }

  entries = dir_entries(fs);
  dir_index = find_dir_entry(fs, name);
  if (dir_index < 0) {
    if ((flags & FS_O_CREATE) == 0u) {
      return FS_ERR_NOT_FOUND;
    }
    dir_index = alloc_dir_entry(fs, name);
    if (dir_index < 0) {
      return FS_ERR_NO_SPACE;
    }
    if (sync_metadata(fs) != FS_OK) {
      return FS_ERR_IO;
    }
  }

  if ((flags & FS_O_TRUNC) != 0u) {
    if (entries[dir_index].first_block != FAT_END &&
        release_chain(fs, entries[dir_index].first_block) != FS_OK) {
      return FS_ERR_STATE;
    }
    entries[dir_index].first_block = FAT_END;
    entries[dir_index].size_bytes = 0u;
    if (sync_metadata(fs) != FS_OK) {
      return FS_ERR_IO;
    }
  }

  fd = alloc_fd(fs);
  if (fd < 0) {
    return FS_ERR_NO_SPACE;
  }

  fs->open_files[fd].in_use = 1;
  fs->open_files[fd].dir_index = (uint8_t)dir_index;
  fs->open_files[fd].offset = 0u;
  fs->open_files[fd].flags = flags;

  return fd;
}

int fs_close(fs_handle_t *fs, int fd) {
  if (validate_common(fs) != FS_OK || !valid_fd(fd)) {
    return FS_ERR_ARG;
  }
  if (fs->open_files[fd].in_use == 0) {
    return FS_ERR_STATE;
  }
  memset(&fs->open_files[fd], 0, sizeof(fs->open_files[fd]));
  return FS_OK;
}

int fs_seek(fs_handle_t *fs, int fd, uint32_t offset) {
  if (validate_common(fs) != FS_OK || !valid_fd(fd)) {
    return FS_ERR_ARG;
  }
  if (fs->open_files[fd].in_use == 0) {
    return FS_ERR_STATE;
  }
  fs->open_files[fd].offset = offset;
  return FS_OK;
}

int fs_read(fs_handle_t *fs, int fd, void *buf, size_t len, size_t *bytes_read) {
  fs_open_file_t *open_file;
  fs_dir_entry_disk_t *entry;
  uint8_t block_buf[FS_BLOCK_SIZE];
  size_t done = 0;

  if (bytes_read != NULL) {
    *bytes_read = 0u;
  }

  if (validate_common(fs) != FS_OK || !valid_fd(fd) || buf == NULL) {
    return FS_ERR_ARG;
  }

  open_file = &fs->open_files[fd];
  if (open_file->in_use == 0u || (open_file->flags & FS_O_READ) == 0u) {
    return FS_ERR_STATE;
  }

  entry = &dir_entries(fs)[open_file->dir_index];
  if (open_file->offset >= entry->size_bytes || len == 0u) {
    return FS_OK;
  }

  if (len > (size_t)(entry->size_bytes - open_file->offset)) {
    len = (size_t)(entry->size_bytes - open_file->offset);
  }

  while (done < len) {
    uint32_t file_offset = open_file->offset;
    uint32_t logical_block = file_offset / FS_BLOCK_SIZE;
    uint32_t intra_block = file_offset % FS_BLOCK_SIZE;
    size_t chunk = FS_BLOCK_SIZE - intra_block;
    uint32_t data_block_index;

    if (chunk > (len - done)) {
      chunk = len - done;
    }

    if (resolve_data_block(fs, entry, logical_block, false, &data_block_index) != FS_OK) {
      return FS_ERR_STATE;
    }
    if (read_data_block(fs, data_block_index, block_buf) != FS_OK) {
      return FS_ERR_IO;
    }

    memcpy((uint8_t *)buf + done, block_buf + intra_block, chunk);
    done += chunk;
    open_file->offset += (uint32_t)chunk;
  }

  if (bytes_read != NULL) {
    *bytes_read = done;
  }
  return FS_OK;
}

int fs_write(fs_handle_t *fs, int fd, const void *buf, size_t len, size_t *bytes_written) {
  fs_open_file_t *open_file;
  fs_dir_entry_disk_t *entry;
  uint8_t block_buf[FS_BLOCK_SIZE];
  size_t done = 0;

  if (bytes_written != NULL) {
    *bytes_written = 0u;
  }

  if (validate_common(fs) != FS_OK || !valid_fd(fd) || buf == NULL) {
    return FS_ERR_ARG;
  }

  open_file = &fs->open_files[fd];
  if (open_file->in_use == 0u || (open_file->flags & FS_O_WRITE) == 0u) {
    return FS_ERR_STATE;
  }

  entry = &dir_entries(fs)[open_file->dir_index];
  while (done < len) {
    uint32_t file_offset = open_file->offset;
    uint32_t logical_block = file_offset / FS_BLOCK_SIZE;
    uint32_t intra_block = file_offset % FS_BLOCK_SIZE;
    size_t chunk = FS_BLOCK_SIZE - intra_block;
    uint32_t data_block_index;

    if (chunk > (len - done)) {
      chunk = len - done;
    }

    if (resolve_data_block(fs, entry, logical_block, true, &data_block_index) != FS_OK) {
      return FS_ERR_NO_SPACE;
    }

    if (read_data_block(fs, data_block_index, block_buf) != FS_OK) {
      return FS_ERR_IO;
    }
    memcpy(block_buf + intra_block, (const uint8_t *)buf + done, chunk);
    if (write_data_block(fs, data_block_index, block_buf) != FS_OK) {
      return FS_ERR_IO;
    }

    done += chunk;
    open_file->offset += (uint32_t)chunk;
  }

  if (open_file->offset > entry->size_bytes) {
    entry->size_bytes = open_file->offset;
  }

  if (sync_metadata(fs) != FS_OK) {
    return FS_ERR_IO;
  }

  if (bytes_written != NULL) {
    *bytes_written = done;
  }
  return FS_OK;
}
