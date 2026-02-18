#include <stdio.h>

#include "fs.h"

int main(int argc, char **argv) {
  const char *image_path;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <image-path>\n", argv[0]);
    return 2;
  }

  image_path = argv[1];
  if (fs_format_image(image_path) != 0) {
    fprintf(stderr, "mkfs failed for %s\n", image_path);
    return 1;
  }

  printf("mkfs: wrote deterministic image %s\n", image_path);
  return 0;
}
