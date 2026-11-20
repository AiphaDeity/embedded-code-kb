#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "boot/image.h"

static size_t read_file(const char *path, uint8_t *data, size_t capacity)
{
  FILE *file = fopen(path, "rb");
  assert(file != NULL);
  const size_t length = fread(data, 1U, capacity, file);
  assert(!ferror(file) && fgetc(file) == EOF && !ferror(file));
  assert(fclose(file) == 0);
  return length;
}

int main(int argc, char **argv)
{
  assert(argc == 3);
  uint8_t header[IMAGE_HEADER_BYTES];
  uint8_t *image = malloc(APP_CAPACITY);
  assert(image != NULL);
  assert(read_file(argv[1], header, sizeof(header)) == sizeof(header));
  const size_t size = read_file(argv[2], image, APP_CAPACITY);
  AppEntry entry = {0};

  // 1. 直接校验实际交叉构建和打包的 App，确认两端的格式、长度和 CRC 相同。
  assert(validate_image(header, image, size, &entry) == IMAGE_OK);
  assert(entry.main_stack == SRAM_END);
  const AppEntry valid = entry;

  // 2. 再依次损坏内容、向量和 header，检查与失败原因对应的返回值。
  image[size - 1U] ^= 0x01U;
  assert(validate_image(header, image, size, &entry) == IMAGE_BAD_CRC);
  image[size - 1U] ^= 0x01U;
  image[4] &= 0xFEU;
  assert(validate_image(header, image, size, &entry) == IMAGE_BAD_VECTOR);
  assert(validate_image(header, image, size - 1U, &entry) == IMAGE_BAD_SIZE);
  header[0] ^= 0x01U;
  assert(validate_image(header, image, size, &entry) == IMAGE_BAD_HEADER);
  assert(entry.main_stack == valid.main_stack && entry.reset_vector == valid.reset_vector);
  free(image);
  puts("Boot image: valid, CRC damage, Thumb bit, truncation, header damage");
  return 0;
}
