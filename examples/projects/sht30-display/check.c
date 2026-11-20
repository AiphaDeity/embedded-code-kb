#include "sample.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
  uint8_t frame[] = {0xBE, 0xEF, 0x92, 0x00, 0x00, 0x00};
  assert(sht30_crc(frame) == 0x92U);
  frame[5] = sht30_crc(frame + 3);
  Sht30Sample sample;
  assert(sht30_decode(frame, &sample));
  assert(sample.temperature_tenths == 855 && sample.humidity_tenths == 0);
  frame[5] ^= 1U;
  assert(!sht30_decode(frame, &sample));
  assert(sample.temperature_tenths == 855 && sample.humidity_tenths == 0);

  frame[0] = frame[1] = 0;
  frame[2] = sht30_crc(frame);
  frame[3] = frame[4] = 0xFF;
  frame[5] = sht30_crc(frame + 3);
  assert(sht30_decode(frame, &sample));
  assert(sample.temperature_tenths == -450 && sample.humidity_tenths == 1000);
  assert(bar_width(-45, 500, 200) == 0 && bar_width(1000, 1000, 200) == 200);
  assert(bar_width(250, 500, 200) == 100 && bar_width(700, 500, 200) == 200);
  puts("SHT30: vendor CRC, rejected frames, signed conversion and bar limits");
}
