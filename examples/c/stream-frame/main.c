#include "decoder.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void check(bool ok)
{
  if (!ok) { fputs("self-check failed\n", stderr); exit(EXIT_FAILURE); }
}

int main(void)
{
  // 1. 先给噪声和半帧，暂时没有结果。
  Decoder decoder = {0};
  const uint8_t first[] = {0x00U, 0xAAU, 2U, 0xAAU};
  for (size_t i = 0; i < sizeof(first); ++i) { check(!decode_byte(&decoder, first[i])); }

  // 2. 送入尾部、一个校验失败帧及一个有效帧，逐个验证交付。
  const uint8_t tail[] = {0x11U, 0xB9U, 0xAAU, 1U, 0x44U, 0x00U, 0xAAU, 1U, 0x22U, 0x23U};
  unsigned frames = 0;
  for (size_t i = 0; i < sizeof(tail); ++i)
  {
    if (decode_byte(&decoder, tail[i]))
    {
      if (frames == 0U)
      {
        check(decoder.length == 2U && decoder.payload[0] == 0xAAU && decoder.payload[1] == 0x11U);
      }
      else
      {
        check(decoder.length == 1U && decoder.payload[0] == 0x22U);
      }
      ++frames;
    }
  }
  check(frames == 2U);
  puts("stream parser: 2 frames");
  return 0;
}
