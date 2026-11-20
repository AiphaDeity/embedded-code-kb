#ifndef CODE_KB_STREAM_DECODER_H
#define CODE_KB_STREAM_DECODER_H

#include <stdbool.h>
#include <stdint.h>

enum { FRAME_SYNC = 0xAA, MAX_PAYLOAD = 8 };
typedef enum { FIND_SYNC, READ_LENGTH, READ_PAYLOAD, READ_CHECKSUM } State;

typedef struct
{
  State state;
  uint8_t length, used, check;
  uint8_t payload[MAX_PAYLOAD];
} Decoder;

// 返回 true 时，payload 和 length 有效，下一次给入字节前先消费。
static inline bool decode_byte(Decoder *decoder, uint8_t byte)
{
  switch (decoder->state)
  {
    // 1. 等待帧头后才把下一字节解释为长度。
    case FIND_SYNC:
      if (byte == FRAME_SYNC) { decoder->state = READ_LENGTH; }
      return false;

    case READ_LENGTH:
      if (byte == FRAME_SYNC) { return false; }
      if (byte == 0U || byte > MAX_PAYLOAD)
      {
        decoder->state = FIND_SYNC;
        return false;
      }
      decoder->length = byte;
      decoder->used = 0U;
      decoder->check = byte;
      decoder->state = READ_PAYLOAD;
      return false;

    // 2. 长度经过校验，按给定数量收数据，包括负载中的帧头值。
    case READ_PAYLOAD:
      decoder->payload[decoder->used++] = byte;
      decoder->check ^= byte;
      if (decoder->used == decoder->length) { decoder->state = READ_CHECKSUM; }
      return false;

    // 3. 只向调用者报告通过校验的帧，错误后按状态继续找帧。
    case READ_CHECKSUM:
      if (byte == decoder->check)
      {
        decoder->state = FIND_SYNC;
        return true;
      }
      decoder->state = byte == FRAME_SYNC ? READ_LENGTH : FIND_SYNC;
      return false;
  }
  return false;
}

#endif
