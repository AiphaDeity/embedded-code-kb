#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"
#include "diskio.h"

enum { BLOCK_BYTES = 512, BLOCKS = 8192 };
static BYTE disk[BLOCKS * BLOCK_BYTES];
static unsigned sync_count, writes;

DSTATUS disk_initialize(BYTE drive) { return drive == 0U ? 0U : STA_NOINIT; }
DSTATUS disk_status(BYTE drive) { return disk_initialize(drive); }

DRESULT disk_read(BYTE drive, BYTE *buffer, DWORD sector, UINT count)
{
  if (drive != 0U || sector >= BLOCKS || count == 0U || count > BLOCKS - sector)
  {
    return RES_PARERR;
  }
  memcpy(buffer, disk + sector * BLOCK_BYTES, count * BLOCK_BYTES);
  return RES_OK;
}

DRESULT disk_write(BYTE drive, const BYTE *buffer, DWORD sector, UINT count)
{
  if (drive != 0U || sector >= BLOCKS || count == 0U || count > BLOCKS - sector)
  {
    return RES_PARERR;
  }
  memcpy(disk + sector * BLOCK_BYTES, buffer, count * BLOCK_BYTES);
  ++writes;
  return RES_OK;
}

DRESULT disk_ioctl(BYTE drive, BYTE command, void *buffer)
{
  (void)buffer;
  if (drive == 0U && command == CTRL_SYNC)
  {
    ++sync_count;
    return RES_OK;
  }
  return RES_PARERR;
}

static void set_word(unsigned offset, unsigned value)
{
  disk[offset] = (BYTE)value;
  disk[offset + 1U] = (BYTE)(value >> 8);
}

static void make_fat16_volume(void)
{
  // 1. 创建用于主机验证的空 FAT16 盘镜像。BPB 写入卷几何，根目录和数据区保留为零。
  disk[0] = 0xEB; disk[1] = 0x3C; disk[2] = 0x90;
  memcpy(&disk[3], "MSDOS5.0", 8U);
  set_word(11U, BLOCK_BYTES);
  disk[13] = 1U;
  set_word(14U, 1U);
  disk[16] = 1U;
  set_word(17U, 512U);
  set_word(19U, BLOCKS);
  disk[21] = 0xF8;
  set_word(22U, 32U);
  memcpy(&disk[54], "FAT16   ", 8U);
  set_word(510U, 0xAA55U);

  // 2. FAT 的前两项须标记为保留。下一次 f_open 从第一个可用簇开始分配。
  set_word(BLOCK_BYTES, 0xFFF8U);
  set_word(BLOCK_BYTES + 2U, 0xFFFFU);
}

int main(void)
{
  FATFS fs;
  // 1. 空白介质应拒绝，应用无格式化或写扇区动作。
  assert(append_log(&fs, "first\r\n", 7U) == FR_NO_FILESYSTEM);
  assert(writes == 0U);

  // 2. 用 SDK 的 FatFs 两次挂载、追加并卸载，再读回合并后的文本。
  make_fat16_volume();
  assert(append_log(&fs, "first\r\n", 7U) == FR_OK);
  assert(append_log(&fs, "second\r\n", 8U) == FR_OK);
  assert(sync_count >= 2U);
  assert(f_mount(&fs, "0:", 1U) == FR_OK);

  FIL input;
  char output[32];
  UINT size = 0U;
  assert(f_open(&input, "0:/KBLOG.TXT", FA_READ) == FR_OK);
  assert(f_read(&input, output, sizeof(output), &size) == FR_OK && size == 15U);
  assert(memcmp(output, "first\r\nsecond\r\n", 15U) == 0);
  assert(f_close(&input) == FR_OK);
  assert(f_mount(NULL, "0:", 0U) == FR_OK);
  puts("FatFs RAM disk: blank media rejected, append twice, sync and read back");
  return 0;
}
