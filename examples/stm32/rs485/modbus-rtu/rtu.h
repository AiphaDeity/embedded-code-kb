#ifndef SKY_MODBUS_RTU_H
#define SKY_MODBUS_RTU_H

#include "../../../c/crc16-modbus/crc16.h"

enum
{
  RTU_LIMIT = 256, RTU_UNIT = 1, RTU_READ = 3,
  RX_INTERCHAR_US = 2865, RX_FRAME_US = 5157
};

typedef struct
{
  uint8_t data[RTU_LIMIT];
  size_t count;
  bool bad;
} RtuFrame;

typedef struct
{
  RtuFrame current;
  uint32_t last_us;
  bool active;
} RtuRx;

// 输入单位为微秒，9600 8E1。两个 RXNE 之间的时间包含一个字符的发送时间。
static inline bool rtu_finish(RtuRx *rx, uint32_t now, RtuFrame *out)
{
  if (!rx->active || (uint32_t)(now - rx->last_us) < RX_FRAME_US)
  {
    return false;
  }
  *out = rx->current;
  rx->current.count = 0U;
  rx->current.bad = false;
  rx->active = false;
  return true;
}

// last_us 来自同一个 32 位计数器，允许单次回绕。out 只在返回 true 时更新。
static inline bool rtu_receive(RtuRx *rx, uint32_t now, uint8_t value, bool uart_error, RtuFrame *out)
{
  // 1. 新字节已跨过帧间隔时，先交付旧帧，再把本字节归入新帧。
  const bool completed = rtu_finish(rx, now, out);

  // 2. 帧内出现过长间隔、串口错误或容量用尽时，整帧视为无效，继续等待帧结束。
  if ((rx->active && (uint32_t)(now - rx->last_us) >= RX_INTERCHAR_US) ||
      uart_error || rx->current.count == RTU_LIMIT)
  {
    rx->current.bad = true;
  }
  else
  {
    rx->current.data[rx->current.count++] = value;
  }
  rx->active = true;
  rx->last_us = now;
  return completed;
}

static inline void build_read_request(uint8_t request[8], uint16_t start, uint16_t count)
{
  request[0] = RTU_UNIT;
  request[1] = RTU_READ;
  request[2] = (uint8_t)(start >> 8);
  request[3] = (uint8_t)start;
  request[4] = (uint8_t)(count >> 8);
  request[5] = (uint8_t)count;
  append_crc(request, 8U, 6U);
}

// 支持单元 1 的功能码 03 和寄存器 0、1。返回应答长度，0 表示静默丢弃。
static inline size_t build_server_reply(const RtuFrame *request, const uint16_t registers[2], uint8_t reply[9])
{
  // 1. 广播、其他站、坏帧或错误校验一律不占用发送总线。
  if (request->bad || request->count < 4U || request->data[0] != RTU_UNIT ||
      !valid_crc(request->data, request->count))
  {
    return 0U;
  }
  const uint8_t function = request->data[1];
  if (function == 0U || (function & 0x80U) != 0U)
  {
    return 0U;
  }

  // 2. 合法请求才读取起始和数量。错误的 PDU 返回对应异常码。
  unsigned exception = 1U;
  unsigned start = 0U, count = 0U;
  if (function == RTU_READ)
  {
    exception = 3U;
    if (request->count == 8U)
    {
      start = ((unsigned)request->data[2] << 8) | request->data[3];
      count = ((unsigned)request->data[4] << 8) | request->data[5];
      if (count > 0U && count <= 125U)
      {
        exception = start < 2U && count <= 2U - start ? 0U : 2U;
      }
    }
  }

  // 3. 普通应答按每寄存器两字节写入，异常应答只写异常码。最后追加 CRC。
  reply[0] = RTU_UNIT;
  reply[1] = exception == 0U ? function : (uint8_t)(function | 0x80U);
  size_t length = 3U;
  if (exception != 0U)
  {
    reply[2] = (uint8_t)exception;
  }
  else
  {
    reply[2] = (uint8_t)(2U * count);
    for (unsigned i = 0; i < count; ++i)
    {
      const uint16_t value = registers[start + i];
      reply[length++] = (uint8_t)(value >> 8);
      reply[length++] = (uint8_t)value;
    }
  }
  append_crc(reply, 9U, length);
  return length + 2U;
}

typedef enum { REPLY_IGNORED, REPLY_VALUES, REPLY_EXCEPTION } ReplyResult;

// 本实验请求固定读两个寄存器。输出仅在相应结果有效时修改。
static inline ReplyResult read_client_reply(const RtuFrame *frame, uint16_t values[2], uint8_t *exception)
{
  if (frame->bad || frame->count < 5U || frame->data[0] != RTU_UNIT ||
      !valid_crc(frame->data, frame->count))
  {
    return REPLY_IGNORED;
  }
  if (frame->data[1] == (RTU_READ | 0x80U) && frame->count == 5U)
  {
    *exception = frame->data[2];
    return REPLY_EXCEPTION;
  }
  if (frame->data[1] == RTU_READ && frame->count == 9U && frame->data[2] == 4U)
  {
    values[0] = (uint16_t)(((unsigned)frame->data[3] << 8) | frame->data[4]);
    values[1] = (uint16_t)(((unsigned)frame->data[5] << 8) | frame->data[6]);
    return REPLY_VALUES;
  }
  return REPLY_IGNORED;
}
#endif
