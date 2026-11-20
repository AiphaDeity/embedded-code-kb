#ifndef SKY_IMAGE_H
#define SKY_IMAGE_H

#include <stddef.h>
#include <stdint.h>
#include "../../../c/common/crc32.h"

#define APP_HEADER_ADDR 0x08004000U
#define APP_ADDR 0x08008000U
#define APP_CAPACITY 0x00078000U
#define SRAM_START 0x20000000U
#define SRAM_END 0x20020000U
#define VECTOR_BYTES 0x188U
#define IMAGE_HEADER_BYTES 16U
#define IMAGE_MAGIC 0x3141424BU
#define IMAGE_VERSION 1U

typedef enum
{
  IMAGE_OK,
  IMAGE_BAD_HEADER,
  IMAGE_BAD_SIZE,
  IMAGE_BAD_VECTOR,
  IMAGE_BAD_CRC,
} ImageResult;

typedef struct
{
  uint32_t main_stack;
  uint32_t reset_vector;
} AppEntry;

static uint32_t read_le32(const uint8_t bytes[4])
{
  return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8) |
         ((uint32_t)bytes[2] << 16) | ((uint32_t)bytes[3] << 24);
}

// header 至少 16 字节，image 指向至少 available 字节。只在 IMAGE_OK 时写 entry。
static inline ImageResult validate_at(const uint8_t *header, const uint8_t *image, uint32_t origin,
                                      size_t available, AppEntry *entry)
{
  // 1. 先核对描述符格式与长度。只在已知能读完整镜像时继续读取向量和 CRC。
  if (read_le32(header) != IMAGE_MAGIC || read_le32(&header[4]) != IMAGE_VERSION)
  {
    return IMAGE_BAD_HEADER;
  }
  const uint32_t size = read_le32(&header[8]);
  if (size < VECTOR_BYTES + 2U || (size_t)size > available || size > UINT32_MAX - origin)
  {
    return IMAGE_BAD_SIZE;
  }

  // 2. 栈顶必须在普通 SRAM 范围且八字节对齐。带 Thumb 位的入口必须指向本镜像代码区。
  const uint32_t stack = read_le32(image);
  const uint32_t reset = read_le32(&image[4]);
  const uint32_t code_addr = reset & ~1U;
  if (stack <= SRAM_START || stack > SRAM_END || (stack & 7U) != 0U || (reset & 1U) == 0U ||
      code_addr < origin + VECTOR_BYTES || code_addr > origin + size - 2U)
  {
    return IMAGE_BAD_VECTOR;
  }

  // 3. 确认文件内容与打包时相同之后才交出 App 入口。
  if (kb_crc32(image, size) != read_le32(&header[12]))
  {
    return IMAGE_BAD_CRC;
  }
  *entry = (AppEntry){.main_stack = stack, .reset_vector = reset};
  return IMAGE_OK;
}

static inline ImageResult validate_image(const uint8_t *header, const uint8_t *image, size_t available, AppEntry *entry)
{
  return validate_at(header, image, APP_ADDR, available < APP_CAPACITY ? available : APP_CAPACITY, entry);
}

#endif
