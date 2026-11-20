#include "decoder.h"

#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

int main(void)
{
#ifdef _WIN32
  if (_setmode(_fileno(stdin), _O_BINARY) < 0) { perror("stdin binary mode"); return EXIT_FAILURE; }
#endif
  Decoder decoder = {0};

  // 1. 从标准输入逐字节读。只处理校验通过且长度恰好为 2 的采样帧。
  int byte;
  while ((byte = getchar()) != EOF)
  {
    if (!decode_byte(&decoder, (uint8_t)byte) || decoder.length != 2U) { continue; }

    // 2. 恢复大端 12 位采样值，再输出一行 JSON，供下游采集。
    const unsigned raw = ((unsigned)decoder.payload[0] << 8U) | decoder.payload[1];
    if (raw > 4095U) { continue; }
    if (printf("{\"adc_raw\":%u}\n", raw) < 0 || fflush(stdout) == EOF)
    {
      perror("stdout");
      return EXIT_FAILURE;
    }
  }
  if (ferror(stdin)) { perror("stdin"); return EXIT_FAILURE; }
  return EXIT_SUCCESS;
}
