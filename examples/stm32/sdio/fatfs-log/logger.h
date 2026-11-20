#ifndef SKY_FATFS_LOGGER_H
#define SKY_FATFS_LOGGER_H

#include "ff.h"

// 文件系统已有 FAT16/FAT32。length 是本行字节数。失败时保留 FatFs 错误码。
static inline FRESULT append_named_log(FATFS *fs, const char *path, const char *line, UINT length)
{
  // 1. 立即挂载卷。只使用已有文件系统，随后在固定日志文件末尾追加。
  FRESULT status = f_mount(fs, "0:", 1U);
  if (status == FR_OK)
  {
    FIL output;
    status = f_open(&output, path, FA_WRITE | FA_OPEN_APPEND);
    if (status == FR_OK)
    {
      // 2. FatFs 即使返回成功，空间不足也可能短写，必须核对实际字节数。
      UINT written = 0U;
      status = f_write(&output, line, length, &written);
      if (status == FR_OK && written != length)
      {
        status = FR_DENIED;
      }

      // 3. f_close 内部同步缓存。写入失败时仍关闭，保留最早的错误。
      const FRESULT closed = f_close(&output);
      if (status == FR_OK)
      {
        status = closed;
      }
    }
  }
  const FRESULT unmounted = f_mount(NULL, "0:", 0U);
  return status == FR_OK ? unmounted : status;
}

static inline FRESULT append_log(FATFS *fs, const char *line, UINT length)
{
  return append_named_log(fs, "0:/KBLOG.TXT", line, length);
}

#endif
